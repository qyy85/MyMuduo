#pragma once

#include"noncopyable.h"
#include"Timestamp.h"

#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;
//muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable{
public:
    //保存每个Channel对象
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller()=default;

    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels)=0;
    virtual void updateChannel(Channel* channel)=0;
    virtual void removeChannel(Channel* channel)=0;

    //判断参数Channel是否在当前的Poller当中
    bool hasChannel(Channel* channel) const;

    //EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    //key:sockfd, value:sockfd对应的channel类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap  Channels_;
private:
    //定义Poller所属的事件循环EventLoop
    EventLoop* ownerLoop_;
};