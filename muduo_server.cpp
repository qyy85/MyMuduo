#include <muduo/net/EventLoop.h>
#include<muduo/net/TcpServer.h>
#include<string>
#include<functional>
#include<iostream>
using namespace std;
using namespace muduo;
using namespace muduo::net;
/*基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针(Reactor反应堆)
3. 明确TcpServer构造函数需要的参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5. 设置合适的服务端线程数量，muduo会自己分配I/o线程和工作线程
*/
class ChatServer{
public:
  ChatServer(EventLoop* loop, //事件循环
            const InetAddress& listenAddr,//ip+port 
            const string& nameArg)//绑定服务器的名字
            :_loop(loop), _server(loop, listenAddr, nameArg)
            {
             //给服务器注册用户连接的创建和断开回调
             _server.setConnectionCallback(bind(&ChatServer::onConnection, this, placeholders::_1));
             //给服务器注册用户读写事件回调 
             _server.setMessageCallback(bind(&ChatServer::onMessage, this, placeholders::_1,
             placeholders::_2, placeholders::_3));
             //设置服务器的线程数量，1个I/O线程 3个工作线程
             _server.setThreadNum(4);
             
            }

            //开启事件循环
            void start(){
               _server.start();
             }
private:
  TcpServer _server;
  EventLoop* _loop; 
//专门处理用户连接创建和断开
  void onConnection(const TcpConnectionPtr& conn){
      if(conn->connected()){
        cout<<conn->peerAddress().toIpPort()<<"-->"<<conn->localAddress().toIpPort()<<"status：online"<<endl;
      }else{
        cout<<conn->peerAddress().toIpPort()<<"-->"<<conn->localAddress().toIpPort()<<"status：offline"<<endl;
        conn->shutdown(); // 相当于close(fd)
      }
  }
  // 专门处理用户的读写事件
  void onMessage(const TcpConnectionPtr& conn, //连接
                 Buffer* buffer, //读写缓冲区
                 Timestamp time) //时间信息
  {
    string buf = buffer->retrieveAllAsString();
    cout<<"recv data:"<<buf<<" time:"<<time.toString()<<endl;
    conn->send(buf);
  }
};

// int main(){
//   EventLoop loop;
//   InetAddress addr("127.0.0.1", 6000);
//   ChatServer server(&loop, addr, "chatserver");
//   server.start(); // listenfd epoll_ctl==>epoll
//   loop.loop(); //epoll_wait以阻塞方式等待新用户的连接，已连接用户的读写事件
//   return 0;
// }