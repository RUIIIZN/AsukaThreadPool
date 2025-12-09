#pragma once

#include <list>
#include <mutex>
#include <condition_variable>
#include <atomic>

const int MaxTaskSize = 200;
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

    bool IsFull() const
    {
        return m_queue.size() >= m_maxSize;
    }
    bool IsEmpty() const
    {
        return m_queue.empty();
    }

    template<typename F>
    void Add(F&& task)
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_notFull.wait(locker,[this]{return m_needStop.load() || !IsFull();});

        if(m_needStop.load()) return;
        m_queue.emplace_back(std::forward<F>(task));
        m_notEmpty.notify_one();
    }

    void Take(T& task)
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_notEmpty.wait(locker,[this]{return m_needStop.load() || !IsEmpty();});

        if(m_needStop.load()) return;
        task = std::move(m_queue.front());
        m_queue.pop_front();
        m_notFull.notify_one();
    }

public:
    SyncQueue(size_t maxSize = MaxTaskSize):
    m_maxSize(maxSize),m_needStop(false){}
    ~SyncQueue(){}

    void AddTask(T&& task)
    {
        Add(std::forward<T>(task));
    }
    void AddTask(const T& task)
    {
        Add(task);
    }
    void TakeTask(T& task)
    {
        Take(task);
    }
    // 停止队列，可选择丢弃未处理任务
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

    bool Empty() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.empty();
    }
    bool Full() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.size() >= m_maxSize;
    }
    size_t Size() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.size();
    }
};