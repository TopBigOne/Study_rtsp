#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <queue>
#include <vector>

#include "Thread.h"
#include "Mutex.h"
#include "Condition.h"

class ThreadPool {
public:
    // 任务类：表示一个线程任务
    class Task {
    public:
        // 函数指针类型：任务的回调函数
        typedef void (*TaskCallback)(void *);

        Task() {};

        // 设置任务回调函数和参数
        void setTaskCallback(TaskCallback cb, void *arg) {
            mTaskCallback = cb;
            mArg = arg;
        }
// 调用回调函数，执行任务
        void handle() {
            if (mTaskCallback)
                mTaskCallback(mArg);
        }

        // 拷贝赋值操作符重载（⚠️这里返回类型应为 Task&，否则无效）
        bool operator=(const Task &task) {
            this->mTaskCallback = task.mTaskCallback;
            this->mArg          = task.mArg;
        }

    private:
        void (*mTaskCallback)(void *);// 回调函数指针

        void *mArg;// 回调参数
    };

    // 工厂方法：创建一个线程池实例
    static ThreadPool *createNew(int num);

    // 构造函数：指定线程数
    ThreadPool(int num);

    ~ThreadPool();

    void addTask(Task &task);

private:
   //  内部线程类：继承自 Thread（假设 Thread 是你定义的线程基类）
    class MThread : public Thread {
    protected:
        // 重写线程的 run 方法
        virtual void run(void *arg);
    };

    // 创建线程
    void createThreads();

    // 取消所有线程
    void cancelThreads();

    // 执行任务处理循环
    void handleTask();

private:
    std::queue<Task> mTaskQueue;       // 任务队列
    Mutex *mMutex;                     // 互斥锁，保护任务队列
    Condition *mCondition;             // 条件变量，用于线程等待/通知
    std::vector<MThread> mThreads;     // 线程容器
    bool mQuit;                        // 线程退出标志
};

#endif //_THREADPOOL_H_