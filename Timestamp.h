#pragma once

#include<iostream>
#include<string>

class Timestamp{
public:
    Timestamp();
    //防止隐式转换，仅需要显示转换有参构造
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    //设置静态函数，返回当前时间，仅属于类
    static Timestamp now();
    //格式化输出时间 yyyy/MM/dd HH:mm:ss
    std::string tostring() const;
private:
    int64_t microSecondsSinceEpoch_;
};