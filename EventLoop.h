#pragma once

#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

#include"CurrentThread.h"
#include"noncopyable.h"
#include"Timestamp.h"
class Channel;
class Poller;

/*
            EventLopp
    ChannelList       Poller
                    
                    ChannelMap<fd, channel*>
*/
//事件循环类
class EventLoop{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();
    //开启事件循环
    void loop();
    //关闭事件循环
    void quit();

    Timestamp pollReturnTime() const{return pollReturnTime_;}

    //在当前loop中执行回调函数cb
    void runInloop(Functor cb);
    //把回调函数cb放入队列，唤醒loop所在线程，执行cb
    void queueInLoop(Functor cb);

    //用来唤醒loop所在线程
    void wakeup();

    //EventLoop的方法 ==》Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const {return threadId_ == CurrentThread::tid();}
private:
    //wake up
    void handleRead(); 
     //执行回调
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; //原子操作,是否在循环
    std::atomic_bool quit_;    // 是否退出循环

    const pid_t threadId_;     //记录当前loop所在线程id

    Timestamp pollReturnTime_;//poller返回发生事件的Channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;//作用：当mianloop获取新的channel，通过轮询算法选择一个subloop,通过此成员唤醒subloop处理
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;//Poller返回的所有感兴趣事件所属的Channel

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否需要执行回调操作
    std::vector<Functor> pendingFunctors_; //存储loop需要执行的所有回调操作
    std::mutex mutex_;//互斥锁，保护上面vector容器的线程安全
};