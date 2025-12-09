#pragma once

#include "./SyncQueue/WorkStealingSyncQueue.hpp"

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

class WorkStealingThreadPool
{
public:
    using Task = std::function<void()>;

private:
    std::vector<std::thread> m_workers;
    SyncQueue<Task> m_taskQueue;
    std::atomic<bool> m_running;
    std::atomic<size_t> m_roundRobin;
    std::once_flag m_flag;
    size_t m_threadnum;

    void Start(int threadnum);
    void RunInThread(size_t index);
    void Stop();

public:
    explicit WorkStealingThreadPool(int threadnum = std::thread::hardware_concurrency());
    ~WorkStealingThreadPool();

    void StopThreadPool();

    void AddTask(Task&& task);
    void AddTask(const Task& task);

    template<typename T, typename... Args>
    auto AddTaskWithReturn(T&& task, Args&&... args) -> std::future<decltype(task(args...))>
    {
        using ReturnType = decltype(task(args...));

        if (!m_running.load())
        {
            return std::future<ReturnType>();
        }

        auto packaged_task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<T>(task), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = packaged_task->get_future();

        Task wrapper_task = [packaged_task]()
        {
            (*packaged_task)();
        };

        AddTask(std::move(wrapper_task));
        return result;
    }
};