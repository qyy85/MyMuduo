#include"TcpServer.h"
#include"InetAddress.h"
#include"Logger.h"
#include"TcpConnection.h"

#include<functional>
#include <strings.h>

static EventLoop* CheckLoopNotNull(EventLoop *loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d mianLoop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
TcpServer::TcpServer(EventLoop *loop, 
                    const InetAddress &listenAddr, 
                    const std::string &nameArg, 
                    Option option)
                    :loop_(CheckLoopNotNull(loop)),
                    ipPort_(listenAddr.toIpPort()),
                    name_(nameArg),
                    acceptor_(new Acceptor(loop, listenAddr, option=KReusePort)),
                    threadPool_(new EventLoopThreadPool(loop, nameArg)),
                    connectionCalback_(),
                    messageCallback_(),
                    nextConnId_(1),
                    started_(0)
{
    //给Acceptor对象设置新连接回调函数
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer(){
    for(auto &item:connections_){
        //定义局部TcpConnectionPtr代替原有指针，作用域仅在本循环内,出作用域自动释放对象资源
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInloop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}
//设置底层subloop的个数，或者说线程的个数
void TcpServer::setThreadNum(int numThreads){
    threadPool_->setThreadNum(numThreads);
}
//开启服务器监听
void TcpServer::start(){
    if(started_++==0){//防止一个TcpServe对象被start多次，只能等于0时进入
        threadPool_->start(threadInitCallback_);
          //acceptor_.get()获得acceptor_对应的指针
        loop_->runInloop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){
    //轮询算法，选择一个subLoop，管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n", name_.c_str(), 
                                                                            connName.c_str(), peerAddr.toIpPort().c_str());
    //通过sockfd获取其绑定的本地ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    //getsockname
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen)<0){
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    //根据连接成功的socket,创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCalback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompeleCallback(writeCompleteCallback_);

    //设置TcpConnection关闭连接回调
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    //TcpConnection::ConnectEstablished连接建立回调函数
    ioLoop->runInloop(std::bind(&TcpConnection::ConnectEstablished, conn));
}
void TcpServer::removeConnection(const TcpConnectionPtr &connP){
    //类对象loop直接调用runInloop执行回调
    loop_->runInloop(std::bind(&TcpServer::removeConnectionInLoop, this, connP));
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &connP){
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] -connection %s\n", name_.c_str(), connP->getName().c_str());
    connections_.erase(connP->getName());
    EventLoop *ioLoop = connP->getLoop();
    //非对象loop调用queueInLoop执行回调，因为可能不是对应线程
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, connP));
}