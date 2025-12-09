#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <list>
#include <mutex>

enum class QueueStatus
{
    OK = 0,
    TIMEOUT = 1,
    STOPPED = 2
};

template<typename T>
class SyncQueue
{
private:
    std::list<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_notFull;
    std::condition_variable m_notEmpty;
    size_t m_maxSize;
    std::atomic<bool> m_needStop;
    size_t m_waitTime; // 超时机制允许线程在无任务时自动退出

    bool IsFull() const
    {
        return m_queue.size() >= m_maxSize;
    }
    bool IsEmpty() const
    {
        return m_queue.empty();
    }

    template<typename F>
    QueueStatus Add(F&& task)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool ready = m_notFull.wait_for(
            lock,
            std::chrono::seconds(m_waitTime),
            [this]{ return m_needStop.load() || !IsFull(); });

        if(!ready) return QueueStatus::TIMEOUT;
        if(m_needStop.load()) return QueueStatus::STOPPED;

        m_queue.emplace_back(std::forward<F>(task));
        m_notEmpty.notify_one();
        return QueueStatus::OK;
    }

    QueueStatus Take(T& task)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool ready = m_notEmpty.wait_for(
            lock,
            std::chrono::seconds(m_waitTime),
            [this]{ return m_needStop.load() || !IsEmpty(); });

        if(!ready) return QueueStatus::TIMEOUT; // 超时返回
        if(m_queue.empty() && m_needStop.load()) return QueueStatus::STOPPED; // 停止状态返回

        task = std::move(m_queue.front());
        m_queue.pop_front();
        m_notFull.notify_one();
        return QueueStatus::OK; // 成功返回
    }

public:
    SyncQueue(size_t maxSize = 200,size_t waitTime = 1)
    :m_maxSize(maxSize),m_needStop(false),m_waitTime(waitTime){}
    ~SyncQueue()
    {
        Stop(true);
    }
    void Stop(bool discardPending = false)
    {
        bool expected = false;
        if (!m_needStop.compare_exchange_strong(expected, true)) 
        { return;} // 已经停止，避免重复停止
           
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            if (discardPending) m_queue.clear();
        }
        m_notFull.notify_all();
        m_notEmpty.notify_all();
    }
    QueueStatus AddTask(T&& task)
    {
        return Add(std::forward<T>(task));
    }
    QueueStatus AddTask(const T& task)
    {
        return Add(task);
    }

    QueueStatus TakeTask(T& task)
    {
        return Take(task);
    }
    size_t Size()const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    bool Empty()const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    bool Full()const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size()>=m_maxSize;
    }
};