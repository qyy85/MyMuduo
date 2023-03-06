#pragma once

#include <vector>
#include <string>
#include<algorithm>

//网络库底层的缓冲区类型定义
/*
    +-----------------+-------------------+-----------------+
    |                 |                   |                 |
    |                 |                   |                 |
    +                 +                   +                 +
    |                 |                   |                 |
    0            readerIndex_         writerIndex          size
*/
class Buffer{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t KInitialSize = 1024;

    explicit Buffer(size_t initialSize = KInitialSize)
                    :buffer_(kCheapPrepend+initialSize),
                    readerIndex_(kCheapPrepend),
                    writeIndex_(kCheapPrepend)
    {}

    //返回读缓冲区的长度
    size_t readableBytes() const{
        return writeIndex_-readerIndex_;
    } 

    //返回写缓冲区的长度
    size_t writableBytes() const{
        return buffer_.size() - writeIndex_;
    }
    //返回预设空间的大小
    size_t prependableBytes() const{
        return readerIndex_;
    }
    //返回缓冲区中可读数据的起始地址
    const char* peek() const{
        return begin() + readerIndex_;
    }
    void retrieve(size_t len){
        if(len < readableBytes()){
            //没有读完，只读了len长度的字符，讲读缓冲区开始下标后移len
            readerIndex_ +=len;
        }else{
            //读完
            retrieveAll();
        }
    }
    void retrieveAll(){
        readerIndex_=writeIndex_=kCheapPrepend;
    }
    //把onMessing函数上报的Buffer数据转成string数据
    std::string retrieveAllAsString(){
        return retrieveAsString(readableBytes());
    }
    std::string retrieveAsString(size_t len){
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    //可写缓冲区的处理
    //当前可写大小，buffer.size()-writeindex_==writableBytes()
    //要写大小len
    void ensureWriteableBytes(size_t len){
        if(writableBytes()<len){
            makeSpace(len);
        }
    }
    //向buffer中添加字符, 将[data, data+len]数据添加到writable缓冲区中
    void append(const char *data, size_t len){
        //先判断是否需要扩容
        ensureWriteableBytes(len);
        //填充数据
        std::copy(data, data+len, beginWrite());
        writeIndex_ +=len;
    }
    char* beginWrite(){
        return begin()+writeIndex_;
    }

    const char* beginWrite() const{
        return begin()+writeIndex_;
    }
    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
private:
    //buffer_vector数组首元素的地址。即数组的起始地址
    char* begin(){
        return &*buffer_.begin();
    }
    //常函数，需要常函数调用
    const char* begin() const{
        return &*buffer_.begin();
    }
    void makeSpace(size_t len){
        if(writableBytes() + prependableBytes() < len + kCheapPrepend){
                buffer_.resize(writeIndex_+len);
        }else{
             size_t  readable = readableBytes();
             std::copy(begin()+readerIndex_, begin()+writableBytes(), begin()+kCheapPrepend);
             readerIndex_ = kCheapPrepend;
             writeIndex_ = readerIndex_+readable;
        }
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;
};