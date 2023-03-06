#pragma once
#include"noncopyable.h"
class InetAddress;

class Socket{
public:
    explicit Socket(int sockfd):sockfd_(sockfd){}
    ~Socket();
    
    int getFd() const {return sockfd_;}
    void bindAddress(const InetAddress &Localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    void shutdownWrite();

    void setTcpNoDelay(bool on); 
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};