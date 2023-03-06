#include"Buffer.h"

#include<errno.h>
#include <sys/uio.h>
#include<unistd.h>
/*
从fd上读取数据， Poller工作在LT模式
Buffer缓冲区使用具体大小的，但从fd上读数据的时候，不知道TCP数据的最终大小
*/
ssize_t Buffer::readFd(int fd, int* saveErrno){
    char extrabuf[65536]={0}; //栈上内存空间，作用域外内存回收
    struct iovec vec[2];
    
    //Buffer缓冲区能写空间大小
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len  = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    //每次最少读取65536==64k的数据
    const int iovcnt = (writable < sizeof extrabuf)?2:1;
    /*
		* 参数： fd    文件描述符
		*       iov    指向iovec结构数组的一个指针
		*       iovcnt  指定了iovec的个数
		* 返回值：函数调用成功时返回读、写的总字节数，失败时返回-1并设置相应的errno。
		* 			writev返回输出的字节总数，通常应等于所有缓冲区长度之和。
		* 			readv返回读到的字节总数。如果遇到文件尾端，已无数据可读，则返回0。
    */
    const size_t n = readv(fd, vec, iovcnt);
    if(n<0){
        *saveErrno = errno;
    }else if(n<=writable){ //Buffer的缓冲区够用
        writeIndex_ += n;
    }else{ //writable不够，extrabuf中也写有部分数据
        writeIndex_ = buffer_.size();
        //从writeIndex_开始写n-writable大小的数据
        append(extrabuf, n-writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno){
    ssize_t n = ::write(fd, peek(), readableBytes());
    if(n<0){
        *saveErrno = errno;
    }
    return n;
}
