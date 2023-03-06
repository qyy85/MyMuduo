#pragma once

#include <unistd.h>
#include<sys/syscall.h>

namespace CurrentThread{
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid(){
        //如果t_cachedTid==0
        if(__builtin_expect(t_cachedTid == 0, 0)){
            //获取线程id
            cacheTid();
        }
        return t_cachedTid;
    }
}