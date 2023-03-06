#include"EpollPoller.h"
#include"Logger.h"
#include"Channel.h"

#include<errno.h>
#include<string.h>
#include<unistd.h>
const int kNew = -1; // 一个channel没有添加到Poller
const int kAdded = 1; //一个Channel已经添加到Poller
const int kDeleted = 2; //一个Channel已经从Poller删除

EpollPoller::EpollPoller(EventLoop* loop)
    :Poller(loop),//设置所属的EventLoop
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),//设置epollfd
    events_(KInitEventListSize)//设置EventList vector的初始大小
{
    if(epollfd_<0){
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EpollPoller::~EpollPoller(){
    close(epollfd_);
}
//epoll_wait的相关操作
//int epoll_wait(int epfd, epoll_event * events, int maxevents, int timeout);
Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels){
    //实际上使用LOG_DEBUG更为合适
    LOG_INFO("func=%s  ==> fd total count:%lu\n", __FUNCTION__, Channels_.size());
    //&*events_.begin 拿到vector数组首地址
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents>0){//有感兴趣的事件
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size()){
            //如果感兴趣事件的数量与events_一样，则对events_进行扩容
            events_.resize(events_.size() * 2);
        }
    }else if(numEvents==0){
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }else{
        //EINTR外部中断，如果是，则继续epoll_wait循环检测，否则
        if(saveErrno != EINTR){
            LOG_ERROR("EpollPoller::poll error!");
        }
    }
    return now;
}
/*
            EventLoop
    ChannelList       Poller
                    
                    ChannelMap<fd, channel*>
*/
//channel update ==>EventLoop updateChannel ==> Poller updateChannel
void EpollPoller::updateChannel(Channel* channel){
    const int index = channel->index();
    LOG_INFO("func=%s fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), channel->index());
    //一种是新添加到Poller中的，一种是状态是删除了，再添加，
    if(index==kNew || index==kDeleted){
        //没有添加到Poller中，现在添加，要放到Poller的ChannelMap中
        if(index==kNew){
            int fd = channel->fd();
            Channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }else{//channel已经在Poller的ChannelMap中，需要进行更新
        int fd = channel->fd();
        //对所有事件不感兴趣
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }else{
            //任然对事件感兴趣，但跟之前不同
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
//channel remove ==>EventLoop removeChannel ==> Poller removeChannel
//从Poller中的ChannelMap中移除
void EpollPoller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    Channels_.erase(fd);
    LOG_INFO("func=%s fd=%d \n", __FUNCTION__, channel->fd());

    int index = channel->index();
    if(index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}
void EpollPoller::fillActiveChannels(int nunEvents, ChannelList* activeChannels) const{
    for(int i=0; i<nunEvents; i++){
        //从感兴趣事件的vector数组中获取每个事件绑定的对应的channel
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        //设置channel对应的感兴趣的事件
        channel->set_revents(events_[i].events);
        //将感兴趣的事件对应的channel添加到Poller中的ChannelList中
        activeChannels->push_back(channel);
    }
}
//更新Channel，主要工作：epoll_ctl add/del/mod
void EpollPoller::update(int operation, Channel* channel){
    epoll_event event;
    memset(&event, 0, sizeof event);
    int fd = channel->fd();
    event.data.fd = fd;
    event.events = channel->events();
    //绑定fd对应的channel
    event.data.ptr = channel;
    //如果出错
    if(::epoll_ctl(epollfd_, operation, fd, &event)<0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }else{
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }

}