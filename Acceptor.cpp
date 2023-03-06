#include"Acceptor.h"
#include"Logger.h"
#include"InetAddress.h"

#include <sys/types.h>         
#include <sys/socket.h>
#include<errno.h>

static int createNonblocking(){
    /*
    SOCK_STREAM 使用流式的传输协议
    SOCK_CLOEXEC 借助文件描述符FD_CLOEXEC 实现子进程运行exec后关闭sock_fd机制
    SOCK_NONBLOCK 借助文件描述符O_NONBLOCK 实现非阻塞IO通信
    */
    int sockfd = ::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
    if(sockfd<0){
        LOG_FATAL("%s:%s:%d listen socket error %d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, 
        const InetAddress& listenAddr, bool reuseport)
        :loop_(loop),
        acceptSocket_(createNonblocking()),//socket()
        acceptChannel_(loop, acceptSocket_.getFd()),
        listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);//bind()
    //注册linstnfd有事件发生对应的回调，此事件就是新连接
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}
Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    listenning_ = true;
    acceptSocket_.listen();
    //注册监听读事件
    acceptChannel_.enableReading();
}
//listenfd有事件发生，就是有新用户的连接
//同时在有事件发生回调函数中，执行后续操作回调函数newConnectionCallback_
void Acceptor::handleRead(){
    //传出参数，里边存储了建立连接的客户端的地址信息
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd>=0){
        if(newConnectionCallback_){
            //轮询查找subloop,唤醒，分发当前新客户端的Channel
            newConnectionCallback_(connfd, peerAddr);
        }else{
            ::close(connfd);
        }
    }else{
        LOG_FATAL("%s:%s:%d accept error %d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE){
        LOG_FATAL("%s:%s:%d socket reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}
