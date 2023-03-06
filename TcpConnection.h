#pragma once
#include"noncopyable.h"
#include"InetAddress.h"
#include"Callback.h"
#include"Buffer.h"
#include"Timestamp.h"

#include<memory>
#include<atomic>
#include<string>

class Channel;
class EventLoop;
class Socket;
/*
    TcpServer ==> Acceptor ==>有新用户连接，通过accept函数拿到通信描述符connfd
    ==> TcpConnection设置回调 ==> Channel ==> Poller ==> Channel执行回调
*/

class TcpConnection:noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string &nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const{return loop_;}
    const std::string& getName() const {return name_;}
    const InetAddress& getLocalAddress() const {return localAddr_;}
    const InetAddress& getPeerAddress() const {return peerAddr_;}

    bool connected() const {return state_ == kConnected;}

    //发送数据
    void send(const std::string &buf);
    //关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb){connectionCalback_ = cb;}
    void setMessageCallback(const MessageCallback &cb){messageCallback_ = cb;}
    void setWriteCompeleCallback(const WriteCompleteCallback &cb){writeCompleteCallback_ = cb;}
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb){highWaterMarkCallback_ = cb;}
    void setCloseCallback(const CloseCallback &cb){closeCallback_ = cb;}

    //连接建立
    void ConnectEstablished();
    //连接销毁
    void connectDestroyed();
    
private:
    enum StateE {
        kDisconnected, //已断开连接
        kConnected, //已连接
        kConnecting, //正在连接
        kDisconnecting //正在断开连接
    };
    void setState(StateE state){state_ = state;}
    
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, int len);
    void shutdownInLoop();

    EventLoop *loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCalback_;//有新连接时的回调
    MessageCallback messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_; //接收数据的缓冲区
    Buffer outputBuffer_;//发送数据的缓冲区

};