#include "../include/CacheThreadPool.h"
void CacheThreadPool::Start(int threadnum)
{
    m_running = true;
    m_currentThreadnum = threadnum;
    m_threadgroup.reserve(threadnum);
    for(int i = 0; i < threadnum; ++i)
    {
      auto thread = std::thread(&CacheThreadPool::RunInThread,this);
      std::thread::id threadId = thread.get_id();
      m_threadgroup.emplace(threadId,std::move(thread));
      m_idelThreadnum++;
    }
}
void CacheThreadPool::RunInThread()
{
    auto tid = std::this_thread::get_id();
    while(m_running.load())
    {
       Task task;
       auto status = m_taskqueue.TakeTask(task);
       if(status == QueueStatus::OK)
       {
        m_idelThreadnum--;
        task();
        m_idelThreadnum++;
       }
       else if(status == QueueStatus::TIMEOUT)
       {
            std::lock_guard<std::mutex> lock(m_mutex);
            if(m_currentThreadnum > m_coreThreadnum)
            {
                auto it = m_threadgroup.find(tid);
                if(it != m_threadgroup.end())
                {
                    if(it->second.joinable())
                    it->second.detach();
                    m_threadgroup.erase(it);
                }
                m_currentThreadnum--;
                m_idelThreadnum--;
                return;
            }
       }
       else if(status == QueueStatus::STOPPED)
       {
            return;
       }
    }
}
void CacheThreadPool::Stop()
{
    if(!m_running.load()){return;}
    
    m_running = false;
    m_taskqueue.Stop(false);  // 停止队列，但不丢弃未处理任务
    
    // 等待所有线程结束
    for(auto& thread : m_threadgroup)
    {
        if(thread.second.joinable())
        {
            thread.second.join();
        }
    }
    m_threadgroup.clear();
}

void CacheThreadPool::AddTask(Task&& task)
{
    if(!m_running.load()) return;
    m_taskqueue.AddTask(std::forward<Task>(task));
}
void CacheThreadPool::AddTask(const Task& task)
{
    if(!m_running.load()) return;
    m_taskqueue.AddTask(task);
}