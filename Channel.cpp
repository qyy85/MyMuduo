#include"Channel.h"
#include"Logger.h"
#include"EventLoop.h"

#include<sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;
//为什么使用指针对象
/*
类的对象:用的是内存栈,是个局部的临时变量.   
类的指针:用的是内存堆,是个永久变量,除非你释放它.   
指针变量是间接访问，但可实现多态（通过父类指针可调用子类对象）
*/
Channel::Channel(EventLoop* loop, int fd)
    :loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1),tied_(false)
{}
Channel::~Channel(){

}

void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}
/*
当改变Channel所表示的fd的events事件后， 
update负责在poller里面更改fd相应的事件， 使用epoll_ctl
*/
void Channel::update(){
    //通过channel所属的Eventloop，调用poller的相应方法， 注册fd的events事件
    loop_->updateChannel(this);
 }
// 在Channel所属的EventLoop中删除当前的Channel
void Channel::remove(){
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){
        //强化弱智能指针
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
            handleEventWitdGuard(receiveTime);
    }else{
            handleEventWitdGuard(receiveTime);
    }
}
//根据poller通知的Channel发生的具体事件， 由Channel负责调用具体的回调函数
void Channel::handleEventWitdGuard(Timestamp receiveTime){
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    //对端正常关闭，EPOLLHUP 表示读写都关闭,不是EPOLLIN(可读)事件
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if(closeCallback_){
            //TcpConnection::handleClose
            closeCallback_();
        }
    }
    if(revents_ & EPOLLERR){
        if(errorCallback_){
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }

    if(revents_ & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }
}