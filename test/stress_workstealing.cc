#include "../ThreadPool/include/WorkStealingThreadPool.h"

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
    std::cout << "=== WorkStealingThreadPool 压力测试 ===" << std::endl;
    WorkStealingThreadPool pool(static_cast<int>(std::thread::hardware_concurrency()));

    auto startTime = std::chrono::high_resolution_clock::now();

    // 测试1: 计算密集任务，验证窃取均衡
    {
        const int taskCount = 800;
        std::vector<std::future<int>> futures;
        futures.reserve(taskCount);
        for (int i = 0; i < taskCount; ++i)
        {
            int start = i * 180;
            int end   = (i + 1) * 180 - 1;
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

    // 测试2: 中等 IO/休眠任务
    {
        const int taskCount = 300;
        std::vector<std::future<void>> futures;
        futures.reserve(taskCount);
        for (int i = 0; i < taskCount; ++i)
        {
            int sleepMs = (i % 40) + 1;
            futures.emplace_back(pool.AddTaskWithReturn(simulateIO, sleepMs));
        }
        for (auto& f : futures) f.wait();
        auto now = std::chrono::high_resolution_clock::now();
        std::cout << "IO 模拟任务 " << taskCount << " 个完成，耗时: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count()
                  << " ms\n";
    }

    // 测试3: 混合任务，观察窃取稳定性
    {
        const int taskCount = 500;
        for (int i = 0; i < taskCount; ++i)
        {
            if (i % 5 == 0)
            {
                pool.AddTask([]{
                    volatile long s = 0;
                    for (int j = 0; j < 9000; ++j) s += j;
                });
            }
            else
            {
                pool.AddTask([]{
                    volatile int x = 0;
                    for (int j = 0; j < 400; ++j) x += j;
                });
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        auto now = std::chrono::high_resolution_clock::now();
        std::cout << "混合任务 " << taskCount << " 个提交完成，总耗时: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count()
                  << " ms\n";
    }

    std::cout << "=== WorkStealingThreadPool 压力测试结束 ===" << std::endl;
    return 0;
}

