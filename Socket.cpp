#include"Socket.h"
#include"InetAddress.h"
#include"Logger.h"

#include<unistd.h>
#include<sys/types.h>          
#include<sys/socket.h>
#include<strings.h>
#include<netinet/tcp.h>
#include<sys/socket.h>

Socket::~Socket(){
    close(sockfd_);
}
void Socket::bindAddress(const InetAddress &localaddr){
    if(0!=-::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in))){
        LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
    }
}
void Socket::listen(){
    if(0!=::listen(sockfd_, 1024)){
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}
int Socket::accept(InetAddress *peeraddr){
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr, sizeof addr);
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_CLOEXEC|SOCK_NONBLOCK);
    if(connfd>=0){
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite(){
    //关闭写端，触发EPOLLHUP事件，此事件不用单独注册
    if(::shutdown(sockfd_, SHUT_WR) < 0){
        LOG_ERROR("shutdownWrite error");
    }
}   

void Socket::setTcpNoDelay(bool on){
    int optval = on?1:0;
    //系统调用
    //获取或者设置与某个套接字关联的选项。选项可能存在于多层协议中，它们总会出现在最上面的套接字层。
    //当操作套接字选项时，选项位于的层和选项的名称必须给出。为了操作套接字层的选项，应该 将层的值指定为SOL_SOCKET。
    //为了操作其它层的选项，控制选项的合适协议号必须给出。例如，为了表示一个选项由TCP协议解析，层应该设定为协议 号TCP。
    /*
    sock：将要被设置或者获取选项的套接字。
    level：选项所在的协议层。
    optname：需要访问的选项名。
    optval：对于getsockopt()，指向返回选项值的缓冲。对于setsockopt()，指向包含新选项值的缓冲。
    optlen：对于getsockopt()，作为入口参数时，选项值的最大长度。作为出口参数时，选项值的实际长度。对于setsockopt()，现选项的长度。
    */
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}
void Socket::setReuseAddr(bool on){
    int optval = on?1:0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
} 
void Socket::setReusePort(bool on){
    int optval = on?1:0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}
void Socket::setKeepAlive(bool on){
    int optval = on?1:0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}