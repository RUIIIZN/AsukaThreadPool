#include "../include/FixedThreadPool.h"

void FixedThreadPool::Start(int threadnum)
{
    m_running = true;
    m_threadgroup.reserve(threadnum);
    
    for(int i = 0; i < threadnum; ++i)
    {
        m_threadgroup.emplace_back(&FixedThreadPool::RunInThread, this);
    }
}

void FixedThreadPool::RunInThread()
{
    while(m_running.load())
    {
        Task task;
        m_taskqueue.TakeTask(task);
        
        // 如果线程池已停止，退出循环
        if(!m_running.load()){ break;}  
        
        // 执行任务
        if(task){task();}
    }
}

void FixedThreadPool::Stop()
{
    if(!m_running.load()){return;}
    
    m_running = false;
    m_taskqueue.Stop(false);  // 停止队列，但不丢弃未处理任务
    
    // 等待所有线程结束
    for(auto& thread : m_threadgroup)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
    m_threadgroup.clear();
}

void FixedThreadPool::StopThreadPool()
{
    std::call_once(m_flag, [this]{Stop();});
}

void FixedThreadPool::AddTask(Task&& task)
{
    if(m_running.load())
    {
        m_taskqueue.AddTask(std::forward<Task>(task));
    }
}

void FixedThreadPool::AddTask(const Task& task)
{
    if(m_running.load())
    {
        m_taskqueue.AddTask(task);
    }
}

