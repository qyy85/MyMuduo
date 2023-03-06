#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include<functional>
#include<mutex>
#include<condition_variable>
#include<string>
class EventLoop;

class EventLoopThread:noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), 
                     const std::string &name=std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    //线程函数
    void threadFunc();
    //每个线程对应的EventLoop
    EventLoop* loop_;
    //是否退出
    bool exiting_;
    //每个EventLoop对应的线程Thread
    Thread thread_;
    //互斥锁
    std::mutex mutex_;
    std::condition_variable cond_;

    //设置函数进行初始化操作，对传入的EventLoop*类型对象
    ThreadInitCallback callback_;
};