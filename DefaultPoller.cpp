#include"Poller.h"
#include"EpollPoller.h"

#include<stdlib.h>
Poller* Poller::newDefaultPoller(EventLoop *loop){
    if(::getenv("MODUO_USE_POLL")){
        return nullptr;//poll的具体实例
    }else{
        return new EpollPoller(loop);//epoll的具体实例
    }
}
