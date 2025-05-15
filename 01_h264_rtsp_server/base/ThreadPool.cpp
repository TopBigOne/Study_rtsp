#include "ThreadPool.h"
#include "Logging.h"
#include "New.h"

ThreadPool* ThreadPool::createNew(int num)
{
    //return new ThreadPool(num);
    return New<ThreadPool>::allocate(num);
}

ThreadPool::ThreadPool(int num) :
    mThreads(num),
    mQuit(false)
{
    mMutex = Mutex::createNew();
    mCondition = Condition::createNew();

    createThreads();
}

ThreadPool::~ThreadPool()
{
    cancelThreads();
    //delete mMutex;
    //delete mCondition;
    Delete::release(mMutex);
    Delete::release(mCondition);
}

void ThreadPool::addTask(ThreadPool::Task& task)
{
    MutexLockGuard mutexLockGuard(mMutex);
    mTaskQueue.push(task);
    mCondition->signal();
}

/**
 * 调用链路：
 *  pthread() -->threadRun()-->run()-->handleTask()-->TaskQueue()-->
 *  Task()#handle()--> MediaSource#taskCallback()-->H264FileMediaSource#readFrame()
 *
 */
void ThreadPool::handleTask()
{
    while(mQuit != true)
    {
        Task task;
        // {} 大括号用于定义一个作用域（scope）。
        // 在这个线程池代码中，临界区的开始和结束使用 {} 是为了精确控制互斥锁的作用范围，这是实现线程安全的关键技术;
        // 临界区开始（通过RAII方式加锁）
        {
            // 创建互斥锁守卫对象，构造函数自动加锁
            MutexLockGuard mutexLockGuard(mMutex);
            if(mTaskQueue.empty())
                mCondition->wait(mMutex);

            // 双重检查：可能在等待期间收到退出信号
            if(mQuit == true)
                break;// 退出循环

            if(mTaskQueue.empty())
                continue;

            // 从队列头部获取任务
            task = mTaskQueue.front();

            mTaskQueue.pop();
        }// 临界区结束（mutexLockGuard析构自动解锁）

        // 执行回调函数:MediaSource#taskCallback()
        task.handle();
    }
}

void ThreadPool::createThreads()
{
    // 1. 创建互斥锁守卫（RAII模式，构造函数自动加锁）
    MutexLockGuard mutexLockGuard(mMutex);

    // 2. 遍历线程向量
    for(std::vector<MThread>::iterator it = mThreads.begin(); it != mThreads.end(); ++it){
        // 3. 启动线程，并传入当前ThreadPool对象指针
        // note:  inner :use pthread()
        (*it).start(this);

        // Note: (*it).join(); 完全可以改为 it->join();
    }

}// 4. 作用域结束，mutexLockGuard析构自动解锁

void ThreadPool::cancelThreads()
{
    MutexLockGuard mutexLockGuard(mMutex);

    mQuit = true;
    mCondition->broadcast();
    for(std::vector<MThread>::iterator it = mThreads.begin(); it != mThreads.end(); ++it){
        (*it).join();
        it->join();
    }


    mThreads.clear();
}

/**
 * pthread() -->threadRun()-->run()-->handleTask()-->TaskQueue()-->Task-->ca
 * @param arg
 */
void ThreadPool::MThread::run(void* arg)
{
    ThreadPool* threadPool = (ThreadPool*)arg;
    threadPool->handleTask();    
}