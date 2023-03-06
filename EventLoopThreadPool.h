#pragma once
#include<functional>
#include"noncopyable.h"

#include<memory>
#include<string>
#include<vector>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool:noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads){numThreads_ = numThreads;}
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();

    bool started() const{return started_;}
    const std::string& name() const{ return name_;}
private:
    //最基础的loop，用户定义的mainLoop
    EventLoop *baseLoop_;

    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    //保存所有创建事件的线程
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
     //保存所有事件线程中的loop的指针
    std::vector<EventLoop*> loops_;
};