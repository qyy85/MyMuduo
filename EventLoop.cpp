#include"EventLoop.h"
#include"Logger.h"
#include"Poller.h"
#include"Channel.h"

#include<sys/eventfd.h>
#include<unistd.h>
#include<errno.h>
#include<memory>

//全局变量，防止一个线程创建多个EventLoop对象
__thread EventLoop *t_loopInThisThread = nullptr;
//定义默认的Poller IO复用接口的超时时间
const int kPollerTimeMs = 10000;

int createEventfd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd<0){
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
            :looping_(false)
            ,quit_(false)
            ,callingPendingFunctors_(false)
            ,threadId_(CurrentThread::tid())
            ,poller_(Poller::newDefaultPoller(this)) //epoll的具体实例
            ,wakeupFd_(createEventfd())
            ,wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread){
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }else{
        t_loopInThisThread = this;
    }
    //设置wakefd的事件类型及事件发生后的回调函数
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    //每个EventLoop都将监听wakeupChannel的EPOLLIN事件
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}
void EventLoop::loop(){
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_){
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollerTimeMs, &activeChannels_);
        for(auto channel:activeChannels_){
            //Poller监听那些channel发生了事件，上报给EventPoll, EventPoll中通知channel处理事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}
void EventLoop::quit(){
    quit_ = true;
    /*
    1 loop在自己的线程中调用quit
    2、loop在非自己线程中调用quit
    */
    if(!isInLoopThread()){
        //唤醒对应线程中的loop,使其跳出循环
        wakeup();
    }
}
void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_DEBUG("EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

//在当前loop中执行回调函数cb
void EventLoop::runInloop(Functor cb){
    if(isInLoopThread()){//在当前loop的线程中，执行回调
        cb();
    }else{//在非当前loop线程中执行回调，就需要唤醒loop所在线程，执行回调
        queueInLoop(cb);
    }
}
//把回调函数cb放入队列，唤醒loop所在线程，执行cb
void EventLoop::queueInLoop(Functor cb){
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb); 
    }
    //唤醒相应需要执行上面回调操作的loop的线程
    //callingPendingFunctors_的意义，当前loop正在执行回调，但loop又有新的回调，在需要唤醒继续执行
    if(!isInLoopThread() || callingPendingFunctors_){
        wakeup();//唤醒loop所在线程
    }
}
 //用来唤醒loop所在线程，对应的设置在构造函数里
void EventLoop::wakeup(){
    //向要唤醒线程，通过wakeupFd_写入数据,wakeupChannel发生读事件，当前loop线程被唤醒
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_DEBUG("EventLoop::handleRead() writes %d bytes instead of 8", n);
    }
}
//EventLoop的方法 ==》Poller的方法
void EventLoop::updateChannel(Channel* channel){
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel){
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel* channel){
    return poller_->hasChannel(channel);
}
void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        //交换两个vector数据，同时解放pendingFunctors_，让其继续其他的各种
        functors.swap(pendingFunctors_);
    }
    for(const Functor &functor : functors){
        functor();//执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;
}
