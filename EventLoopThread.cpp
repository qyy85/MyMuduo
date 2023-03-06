
#include"EventLoopThread.h"
#include"EventLoop.h"
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb , 
                                   const std::string &name)
                                   :loop_(nullptr)
                                   ,exiting_(false)
                                   ,thread_(std::bind(&EventLoopThread::threadFunc, this), name)
                                   ,mutex_()
                                   ,cond_()
                                   ,callback_(cb)

{

}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_ != nullptr){
        //loop退出
        loop_->quit();
        //等待子线程结束 
        thread_.join();
    }
}
EventLoop* EventLoopThread::startLoop(){
    //调用底层thread的类，构造并启动新线程，执行回调函数threadFunc
    thread_.start();
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr){
            //等threadFunc中的loop_不为空
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc(){
    EventLoop loop; //创建一个独立的EventLoop，和线程一一对应，one loop per thread
    if(callback_){
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();

    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}