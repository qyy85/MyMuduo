#include"TcpConnection.h"
#include"Logger.h"
#include"Socket.h"
#include"Channel.h"
#include"EventLoop.h"

#include<functional>
#include<unistd.h>
#include<sys/types.h>          
#include<sys/socket.h>
#include<strings.h>
#include<netinet/tcp.h>
#include<sys/socket.h>

static EventLoop* CheckLoopNotNull(EventLoop *loop){
    if(loop==nullptr){
        LOG_FATAL("%s:%s:%d mianLoop is null \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
TcpConnection::TcpConnection(EventLoop *loop, const std::string &nameArg, 
                            int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
                            :loop_(CheckLoopNotNull(loop)),
                            name_(nameArg),
                            state_(kConnecting),
                            reading_(true),
                            socket_(new Socket(sockfd)),
                            channel_(new Channel(loop, sockfd)),
                            localAddr_(localAddr),
                            peerAddr_(peerAddr),
                            highWaterMark_(64*1024*1024)//64M
{
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->seterrorCallback(std::bind(&TcpConnection::handleError, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true); 
}
TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), state_.load());
}

void TcpConnection::handleRead(Timestamp receiveTime){
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if(n>0)
    {//有数据
        //已建立连接的用户，有可读事件发生，调用用户传入的回调操作onMessage()
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n==0){
        handleClose();
    }else{
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}
//针对缓冲区进行操作
void TcpConnection::handleWrite(){
    if(channel_->isWriting()){
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if(n>0){
            outputBuffer_.retrieve(n);
            //发送完成
            if(outputBuffer_.readableBytes()==0){
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    //唤醒loop_对应线程，执行回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                //在数据未发送完成中调用shutdown(),会将state_置为kDisconnecting
                //当数据发送完成后，在此处真正关闭
                if(state_ == kDisconnecting){
                    shutdownInLoop();
                }
            }
        }else{
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }else{
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}
//poller返回连接关闭事件(EPOLLHUP) ==>Channel的closeCallback_() ==> Tcpconnection的handleClose()
void TcpConnection::handleClose(){
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), state_.load());
    setState(kDisconnected);
    channel_->disableAll();

    //当前TcpConnection对象指针
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCalback_(connPtr);//执行连接关闭的回调
    closeCallback_(connPtr);//关闭连接回调，执行的是TcpServer::removeConnection回调方法
}
void TcpConnection::handleError(){
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    //系统调用
    /*
    sock：将要被设置或者获取选项的套接字。
    level：选项所在的协议层。
    optname：需要访问的选项名。
    optval：对于getsockopt()，指向返回选项值的缓冲。对于setsockopt()，指向包含新选项值的缓冲。
    optlen：对于getsockopt()，作为入口参数时，选项值的最大长度。作为出口参数时，选项值的实际长度。对于setsockopt()，现选项的长度。
        level指定控制套接字的层次.可以取三种值:
            1)SOL_SOCKET:通用套接字选项.
            2)IPPROTO_IP:IP选项.
            3)IPPROTO_TCP:TCP选项.　
    */
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen)<0){
        err = errno; 
    }else{
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}
//针对用户
void TcpConnection::send(const std::string &buf){
    if(state_ == kConnected){
        if(loop_->isInLoopThread()){
            //c_str() string ==> char*
            sendInLoop(buf.c_str(), buf.size());
        }else{
            loop_->runInloop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}
/*
发送数据底层函数
应用写的快， 而内核发送数据慢， 需要把待发送数据写入到缓冲区，且设置水位回调
*/
void TcpConnection::sendInLoop(const void *data, int len){
    ssize_t nwrote = 0;
    //剩余未写数据长度
    size_t remaining = len;
    bool faultError = false;
    //之前调用过该TcpConnection的shutdown，不再进行发送
    if(state_==kDisconnected){
        LOG_ERROR("disconnected, give up writing");
        return;    
    }
    //表示channel_首次开始写数据，而且缓冲区没有待发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes()==0){
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote>=0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                //数据已经发送完成， 对应的channel不再设置epollout事件
                //函数地址+对象地址
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }else{ //nwrote<0;
            nwrote = 0;
            if(errno != EWOULDBLOCK){//EWOULDBLOCK简单理解是没有写完，需要再次写
                LOG_ERROR("TcpConnection::sendInLoop");
                //对端sockfd重置，重新连接
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }
    /*
    1.首次write没有把数据全部发送出去，剩余数据需要保存到缓冲区中
    2.channel注册epollout事件，poller发现tcp的发送缓冲区有空间， 
    3.通知相应的sock-channel,执行writeCallback_回调方法，即TcpConnection::handleWrite()方法，把缓冲区中数据发送完成
    */
    if(!faultError && remaining>0){ 
        //发送缓冲区现有剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_ && oldLen<highWaterMark_ && highWaterMarkCallback_){
            //设置高水位回调函数
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen+remaining));
        }
        //剩余写到发送缓冲区
        outputBuffer_.append((char*)data+nwrote, remaining);
        if(!channel_->isWriting()){
            channel_->enableWriting();//设置channel写事件，否则poller不能给channel注册epollout事件
        }
    }

}
//连接建立
void TcpConnection::ConnectEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());//这里的操作是connection存在，channel进行感兴趣事件或者说发生事件的相应回调
    channel_->enableReading();//向poller注册channel的epollin事件

    //新连接建立，执行回调
    connectionCalback_(shared_from_this());
}
//连接销毁
void TcpConnection::connectDestroyed(){
    if(state_==kConnected){
        setState(kDisconnected);
        channel_->disableAll();//设置对所有事件都不感兴趣，并提交到poller中

        connectionCalback_(shared_from_this());
    }
    channel_->remove();//从poller中删除
}
 //关闭连接
void TcpConnection::shutdown(){
    if(state_ == kConnected){
        setState(kDisconnecting);
        loop_->runInloop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}
void TcpConnection::shutdownInLoop(){
    //
    if(!channel_->isWriting()){
        socket_->shutdownWrite();//关闭写端
    }
}