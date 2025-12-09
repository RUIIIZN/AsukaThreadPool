#pragma once

#include "./SyncQueue/FixedSyncQueue.hpp"
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <vector>

class FixedThreadPool
{
public:
    using Task = std::function<void()>;
private:
    std::vector<std::thread> m_threadgroup; 
    SyncQueue<Task> m_taskqueue;
    std::atomic<bool> m_running;
    std::once_flag m_flag;

    void Start(int threadnum);
    void RunInThread();
    void Stop();
public:
    FixedThreadPool(int threadnum = std::thread::hardware_concurrency())
    :m_taskqueue(MaxTaskSize),m_running(false)
    {
        Start(threadnum);
    }
    ~FixedThreadPool(){Stop();}

    void StopThreadPool();

    void AddTask(Task&& task);
    void AddTask(const Task& task);

    template<typename T,typename... Args>
    auto AddTaskWithReturn(T&& task,Args&&... args)->std::future<decltype(task(args...))>
    {
        using ReturnType = decltype(task(args...));
        
        // 如果线程池未运行，返回无效的 future
        if(!m_running.load())
        {
            return std::future<ReturnType>();
        }
        
        // 使用 packaged_task 包装任务，可以获取返回值
        auto packaged_task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<T>(task), std::forward<Args>(args)...)
        );
        
        // 获取 future 用于后续获取返回值
        std::future<ReturnType> result = packaged_task->get_future();
        
        // 将 packaged_task 包装成 void() 类型的 Task，以便放入队列
        Task wrapper_task = [packaged_task]() {
            (*packaged_task)();  // 执行任务，结果会自动设置到 future 中
        };
        
        // 将任务加入队列
        m_taskqueue.AddTask(std::move(wrapper_task));
        
        return result;
    }
};
