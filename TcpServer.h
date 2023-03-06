#pragma once

#include"noncopyable.h"
#include"EventLoop.h"
#include"Acceptor.h"
#include"EventLoopThreadPool.h"
#include"Callback.h"
#include"TcpConnection.h"
#include"Buffer.h"

#include<string>
#include<memory>
#include<atomic>
#include<map>

class TcpServer:noncopyable{
public:
    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    enum Option{
        kNoReuseport,
        KReusePort,
    };
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = kNoReuseport);
    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback &cb){threadInitCallback_ = cb;}
    void setConnectionCallback(const ConnectionCallback &cb){connectionCalback_ = cb;}
    void setMessageCallback(const MessageCallback &cb){messageCallback_ = cb;}
    void setWriteCompeleCallback(const WriteCompleteCallback &cb){writeCompleteCallback_ = cb;}

    //设置底层subloop的个数，或者说线程的个数
    void setThreadNum(int numThreads);
    //开启服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &connP);
    void removeConnectionInLoop(const TcpConnectionPtr &connP);

    EventLoop* loop_; //baseLoop,或者说MainLoop
    
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;//运行在MainLoop,监听新连接事件 
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCalback_;//有新连接时的回调
    MessageCallback messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完以后的回调

    ThreadInitCallback threadInitCallback_;//loop线程初始化的回调
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;//保存所有连接

};