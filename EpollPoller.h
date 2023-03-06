#pragma once

#include"Poller.h"

#include <vector>
#include <sys/epoll.h>
/*
1. 创建epoll实例对象 epoll_create
2. 将用于监听的套接字添加到epoll实例中epoll_event, epoll_ctl 
3. 检测添加到epoll实例中的文件描述符是否已就绪，并将这些已就绪的文件描述符进行处理 epoll_wait
    * 如果是监听的文件描述符，和新客户端建立连接，将得到的文件描述符添加到epoll实例中
    * 如果是通信的文件描述符，和对应的客户端通信，如果连接已断开，将该文件描述符从epoll实例中删除
4. 重复第 3 步的操作
*/
//一个EventLoop 对应一个Poller,多个channel
class EpollPoller :public Poller{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override;
    //重写基类方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    //EventList  = std::vector<epoll_event>初始化长度
    static const int KInitEventListSize = 16;
    //填充channel在具体发生事件revents_,并将channel添加到 ChannelList
    void fillActiveChannels(int nunEvents, ChannelList* activeChannels) const;
    //更新channel通道 
    void update(int operation, Channel* channel);

    using EventList  = std::vector<epoll_event>;
    //epoll实例对象
    int epollfd_;
    //关注事件集合，epoll_wait的传出参数
    EventList events_;
};