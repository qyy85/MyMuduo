#pragma once
#include<functional>
#include<memory>
#include"noncopyable.h"
#include"Timestamp.h"
class EventLoop;

/*
Channel类 是一个通道， 封装sockfd文件描述符和对应的感兴趣的事件，如EPOLLIN，EPOLLOUT事件
同时绑定返回的具体事件
*/
class Channel:noncopyable{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象
    void setReadCallback(ReadEventCallback cd){readCallback_ = std::move(cd);}
    void setWriteCallback(EventCallback cd){writeCallback_ = std::move(cd);}
    void setCloseCallback(EventCallback cd){closeCallback_ = std::move(cd);}
    void seterrorCallback(EventCallback cd){errorCallback_ = std::move(cd);}

    //防止当channel被手动remove，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const{return fd_;}
    int events() const{return events_;}
    void set_revents(int revt){revents_ = revt;}

    //设置fd相应的事件状态,并更新到epoll中
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() {events_ &= ~kReadEvent; update();}
    void enableWriting(){events_ |= kWriteEvent; update();}
    void disableWriting(){events_ &= ~kWriteEvent; update();}
    void disableAll() {events_ = kNoneEvent; update();}

    //返回fd当前的事件状态
     bool isNoneEvent() const {return events_ == kNoneEvent;}
     bool isWriting() const {return events_ & kWriteEvent;}
     bool isReading() const {return events_ & kReadEvent;}
     bool inNoneEvent() const{return events_ == kNoneEvent;}

     int index(){return index_;}
     void set_index(int idx){index_ = idx;}

    //返回本channel属于的EventLoop
    EventLoop* ownerLoop(){return loop_;}   
    void remove();  
private:
    void update();
    void handleEventWitdGuard(Timestamp receiveTime);
    //感兴趣事件的状态描述
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; //事件循环
    const int fd_;   // 通信文件描述符
    int events_;    // 注册fd感兴趣事件
    int revents_;   // 返回具体发生的事件
    int index_;      

    std::weak_ptr<void> tie_;
    bool tied_;
//因为Channel通道中课获得fd最终发生的具体事件revents,所有可以针对的调用对应的回调函数
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};