#pragma once
#include<functional>

#include"noncopyable.h"
#include"EventLoop.h"
#include"Socket.h"
#include"Channel.h"

class Acceptor:noncopyable{
public:
    using NewConnectionCallback_ = std::function<void(int sokefd, const InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback_ &cb){
        newConnectionCallback_ = cb;
    }
    bool listenning() const {return listenning_;}
    void listen();

private:
    void handleRead();

    EventLoop* loop_;//用户定义的baseLoop 也是MianLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback_ newConnectionCallback_;
    bool listenning_;
};