#include"InetAddress.h"
#include<strings.h>
#include<string.h>
InetAddress::InetAddress(uint16_t port, std::string ip){
    //初始化缓冲区
    bzero(&addr_, sizeof(addr_));
    //设置模式IPV4
    addr_.sin_family = AF_INET;
    //设置端口，htons主机字节序转换到网络字节序（小端--->大端）
    addr_.sin_port = htons(port);
    //设置ip， inet_addr仅处理ipv4的ip地址， 小端--->大端
    //c_str() string-->char *
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const{
    char buf[64]={0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;

}
std::string InetAddress::toIpPort() const{
    char buf[64]={0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    //参数1相当于一个指针，指向的下标就是写向字符串的位置
    sprintf(buf+end, ":%u", port);
    return buf;
}
uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}
