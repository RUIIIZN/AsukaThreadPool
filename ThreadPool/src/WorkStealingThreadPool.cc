#include "../include/WorkStealingThreadPool.h"

#include <algorithm>

namespace
{
inline size_t NormalizeThreadNum(int threadnum)
{
    if (threadnum <= 0)
    {
        auto hc = std::thread::hardware_concurrency();
        return hc == 0 ? 1u : static_cast<size_t>(hc);
    }
    return static_cast<size_t>(threadnum);
}
}

void WorkStealingThreadPool::Start(int threadnum)
{
    m_running = true;
    m_threadnum = static_cast<size_t>(threadnum);
    m_workers.reserve(threadnum);

    for (size_t i = 0; i < static_cast<size_t>(threadnum); ++i)
    {
        m_workers.emplace_back(&WorkStealingThreadPool::RunInThread, this, i);
    }
}

WorkStealingThreadPool::WorkStealingThreadPool(int threadnum)
    : m_taskQueue(NormalizeThreadNum(threadnum)),
      m_running(false),
      m_roundRobin(0),
      m_threadnum(NormalizeThreadNum(threadnum))
{
    Start(static_cast<int>(m_threadnum));
}

WorkStealingThreadPool::~WorkStealingThreadPool()
{
    Stop();
}

void WorkStealingThreadPool::RunInThread(size_t index)
{
    while (m_running.load())
    {
        Task task;
        auto status = m_taskQueue.TakeTask(task, index);
        if (status == QueueStatus::OK)
        {
            task();
        }
        else if (status == QueueStatus::STOPPED)
        {
            break;
        }
        // TIMEOUT 情况下继续等待，避免忙等
    }
}

void WorkStealingThreadPool::Stop()
{
    if (!m_running.load()) return;

    m_running = false;
    m_taskQueue.Stop(false);

    for (auto& t : m_workers)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    m_workers.clear();
}

void WorkStealingThreadPool::StopThreadPool()
{
    std::call_once(m_flag, [this]{ Stop(); });
}

void WorkStealingThreadPool::AddTask(Task&& task)
{
    if (!m_running.load()) return;
    size_t bucket = m_roundRobin.fetch_add(1) % m_threadnum;
    m_taskQueue.AddTask(std::forward<Task>(task), bucket);
}

void WorkStealingThreadPool::AddTask(const Task& task)
{
    if (!m_running.load()) return;
    size_t bucket = m_roundRobin.fetch_add(1) % m_threadnum;
    m_taskQueue.AddTask(task, bucket);
}

