#include "../ThreadPool/include/CacheThreadPool.h"

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

int countPrimes(int start, int end)
{
    int count = 0;
    for (int i = start; i <= end; ++i)
    {
        if (i < 2) continue;
        bool isPrime = true;
        for (int j = 2; j * j <= i; ++j)
        {
            if (i % j == 0)
            {
                isPrime = false;
                break;
            }
        }
        if (isPrime) ++count;
    }
    return count;
}

void simulateIO(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

int main()
{
    std::cout << "=== CacheThreadPool 压力测试 ===" << std::endl;
    CacheThreadPool pool(4, std::thread::hardware_concurrency() * 2);

    auto startTime = std::chrono::high_resolution_clock::now();

    // 测试1: 计算密集任务
    {
        const int taskCount = 600;
        std::vector<std::future<int>> futures;
        futures.reserve(taskCount);
        for (int i = 0; i < taskCount; ++i)
        {
            int start = i * 150;
            int end   = (i + 1) * 150 - 1;
            futures.emplace_back(pool.AddTaskWithReturn(countPrimes, start, end));
        }
        int total = 0;
        for (auto& f : futures) total += f.get();
        auto now = std::chrono::high_resolution_clock::now();
        std::cout << "计算任务 " << taskCount << " 个完成，素数总数: "
                  << total << "，耗时: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count()
                  << " ms\n";
    }

    // 测试2: 短 IO/休眠任务，观察弹性线程扩展
    {
        const int taskCount = 400;
        std::vector<std::future<void>> futures;
        futures.reserve(taskCount);
        for (int i = 0; i < taskCount; ++i)
        {
            int sleepMs = (i % 20) + 1;
            futures.emplace_back(pool.AddTaskWithReturn(simulateIO, sleepMs));
        }
        for (auto& f : futures) f.wait();
        auto now = std::chrono::high_resolution_clock::now();
        std::cout << "IO 模拟任务 " << taskCount << " 个完成，耗时: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count()
                  << " ms\n";
    }

    // 测试3: 混合任务，触发线程回收
    {
        const int taskCount = 500;
        for (int i = 0; i < taskCount; ++i)
        {
            if (i % 4 == 0)
            {
                pool.AddTask([]{
                    volatile long s = 0;
                    for (int j = 0; j < 8000; ++j) s += j;
                });
            }
            else
            {
                pool.AddTask([]{
                    volatile int x = 0;
                    for (int j = 0; j < 300; ++j) x += j;
                });
            }
        }

        // 给线程一些时间执行和回收空闲线程
        std::this_thread::sleep_for(std::chrono::seconds(KeepAliveTime + 2));
        auto now = std::chrono::high_resolution_clock::now();
        std::cout << "混合任务 " << taskCount << " 个提交完成，总耗时: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count()
                  << " ms\n";
    }

    std::cout << "=== CacheThreadPool 压力测试结束 ===" << std::endl;
    return 0;
}

