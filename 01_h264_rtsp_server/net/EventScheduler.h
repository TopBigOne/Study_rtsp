#ifndef _EVENT_SCHEDULER_H_
#define _EVENT_SCHEDULER_H_
#include <vector>
#include <queue>

#include "poller/PollPoller.h"
#include "Timer.h"
#include "Mutex.h"

/**
 * 事件调度器类，负责管理并调度各种事件
 */
class EventScheduler
{
public:
    typedef void (*Callback)(void*);// 定义回调函数类型

    // 支持的轮询器类型枚举
    enum PollerType
    {
        POLLER_SELECT,  // select 轮询器
        POLLER_POLL,    // poll 轮询器
        POLLER_EPOLL    // epoll 轮询器（Linux特有）
    };

    // 静态工厂方法，用于创建事件调度器实例
    static EventScheduler* createNew(PollerType type);

    // 构造函数（指定轮询器类型和文件描述符）
    EventScheduler(PollerType type, int fd);
    virtual ~EventScheduler();  // 虚析构函数

    /* 事件管理接口 */
// 添加触发事件（立即执行的事件）
    bool addTriggerEvent(TriggerEvent* event);

    // 添加定时事件：延迟执行
    Timer::TimerId addTimedEventRunAfater(TimerEvent* event, Timer::TimeInterval delay);

    // 添加定时事件：在指定时间点执行
    Timer::TimerId addTimedEventRunAt(TimerEvent* event, Timer::Timestamp when);

    // 添加定时事件：周期性执行
    Timer::TimerId addTimedEventRunEvery(TimerEvent* event, Timer::TimeInterval interval);

    // 移除定时事件
    bool removeTimedEvent(Timer::TimerId timerId);

    // 添加I/O事件
    bool addIOEvent(IOEvent* event);

    // 更新I/O事件
    bool updateIOEvent(IOEvent* event);

    // 移除I/O事件
    bool removeIOEvent(IOEvent* event);

    // 主事件循环
    void loop();

    // 唤醒事件循环（通常用于跨线程唤醒）
    void wakeup();

    // 在本地线程中执行回调函数（线程安全）
    void runInLocalThread(Callback callBack, void* arg);

    // 处理其他事件（非I/O、非定时事件）
    void handleOtherEvent();

private:
    // 处理所有触发事件
    void handleTriggerEvents();

    // 静态读回调函数（用于唤醒机制）
    static void handleReadCallback(void*);

    // 实际的读处理函数
    void handleRead();

private:
    bool mQuit;                     // 退出标志
    Poller* mPoller;                // 轮询器指针（实际可能是SelectPoller/PollPoller/EpollPoller）
    TimerManager* mTimerManager;    // 定时器管理器
    std::vector<TriggerEvent*> mTriggerEvents; // 触发事件列表

    int mWakeupFd;                  // 用于唤醒事件循环的文件描述符
    IOEvent* mWakeIOEvent;          // 与唤醒机制相关的I/O事件

    std::queue<std::pair<Callback, void*> > mCallBackQueue; // 回调函数队列
    Mutex* mMutex;                  // 互斥锁（用于线程安全）
};

#endif //_EVENT_SCHEDULER_H_