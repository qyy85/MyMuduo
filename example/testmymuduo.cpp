#include<mymuduo/TcpConnection.h>
#include<mymuduo/TcpServer.h>
#include<mymuduo/Logger.h>

#include<string>
#include<functional>

class EchoServer{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name):
                loop_(loop),server_(loop, addr, name)
    {
        //注册回到函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, 
                                    std::placeholders::_2, std::placeholders::_3));
        server_.setThreadNum(3);
    }
    void start(){
        server_.start();
    }
private:
    EventLoop *loop_;
    TcpServer server_;
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->connected()){
            LOG_INFO("Connection UP : %s", conn->getPeerAddress().toIpPort().c_str());
        }else{
            LOG_INFO("Connection DOWN : %s", conn->getPeerAddress().toIpPort().c_str());    
        }
    }
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time){
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); //关闭写端，触发EPOLLHUP事件，调用colseCallback_
    }
};

int main(){
    EventLoop loop;
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "qian-01");// Acceptor non-blocking(非阻塞)listenfd create bind(通过Socket类)
    server.start(); // 启动线程池，开启acceptor listen, 将打包loop与listenfd的acceptChannel注册到mainloop，监听新连接事件
    loop.loop();//启动mainLoop
    return 0;
}