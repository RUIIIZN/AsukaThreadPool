#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

enum class QueueStatus
{
    OK = 0,
    TIMEOUT = 1,
    STOPPED = 2
};

// 为工作窃取线程池准备的多桶队列，每个线程拥有自己的队列，空闲时可从其他桶窃取
template<typename T>
class SyncQueue
{
private:
    std::vector<std::deque<T>> m_queues;
    size_t m_maxsize;      // 每个桶的最大容量
    size_t m_bucketCount;
    size_t m_waitTime;     // wait_for 的超时时间（秒）

    mutable std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;
    std::atomic<bool> m_needStop;

    bool IsFull(const size_t index) const
    {
        return m_queues[index].size() >= m_maxsize;
    }
    bool IsEmpty(const size_t index) const
    {
        return m_queues[index].empty();
    }
    size_t TotalSizeUnsafe() const
    {
        size_t size = 0;
        for (const auto& q : m_queues)
        {
            size += q.size();
        }
        return size;
    }

    bool PopFromOwn(size_t bucket, T& task)
    {
        if (m_queues[bucket].empty()) return false;
        task = std::move(m_queues[bucket].back()); // 自己使用后进先出，提升缓存局部性
        m_queues[bucket].pop_back();
        return true;
    }
    bool StealFromOthers(size_t bucket, T& task)
    {
        for (size_t i = 0; i < m_bucketCount; ++i)
        {
            if (i == bucket || m_queues[i].empty()) continue;
            task = std::move(m_queues[i].front()); // 窃取使用先进先出，减少竞争
            m_queues[i].pop_front();
            return true;
        }
        return false;
    }

    template<typename F>
    QueueStatus AddInternal(F&& task, const size_t bucket)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool ready = m_notFull.wait_for(
            lock,
            std::chrono::seconds(m_waitTime),
            [this, bucket]
            {
                return m_needStop.load() || !IsFull(bucket);
            });

        if (!ready) return QueueStatus::TIMEOUT;
        if (m_needStop.load()) return QueueStatus::STOPPED;

        m_queues[bucket].emplace_back(std::forward<F>(task));
        m_notEmpty.notify_one();
        return QueueStatus::OK;
    }

public:
    SyncQueue(size_t bucketCount, size_t maxsize = 200, size_t waitTime = 1)
        : m_maxsize(maxsize),
          m_bucketCount(bucketCount),
          m_waitTime(waitTime),
          m_needStop(false)
    {
        m_queues.resize(bucketCount);
    }
    ~SyncQueue()
    {
        Stop(true);
    }

    QueueStatus AddTask(T&& task, const size_t bucket)
    {
        return AddInternal(std::forward<T>(task), bucket);
    }
    QueueStatus AddTask(const T& task, const size_t bucket)
    {
        return AddInternal(task, bucket);
    }

    QueueStatus TakeTask(T& task, size_t bucket)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool ready = m_notEmpty.wait_for(
            lock,
            std::chrono::seconds(m_waitTime),
            [this]
            {
                return m_needStop.load() || TotalSizeUnsafe() > 0;
            });

        if (!ready) return QueueStatus::TIMEOUT;
        if (m_needStop.load()) return QueueStatus::STOPPED;

        if (!PopFromOwn(bucket, task))
        {
            if (!StealFromOthers(bucket, task))
            {
                // 理论上在 ready 为 true 时不应出现，但为安全起见返回 TIMEOUT
                return QueueStatus::TIMEOUT;
            }
        }

        m_notFull.notify_one();
        return QueueStatus::OK;
    }

    void Stop(bool discardPending = false)
    {
        bool expected = false;
        if (!m_needStop.compare_exchange_strong(expected, true))
        {
            return; // 已经停止
        }

        {
            std::lock_guard<std::mutex> locker(m_mutex);
            if (discardPending)
            {
                for (auto& q : m_queues) q.clear();
            }
        }
        m_notFull.notify_all();
        m_notEmpty.notify_all();
    }

    bool Full(const size_t index) const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return IsFull(index);
    }
    bool Empty(const size_t index) const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return IsEmpty(index);
    }
    size_t Size() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return TotalSizeUnsafe();
    }
};