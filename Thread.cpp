#include"Thread.h"
#include"CurrentThread.h"
#include<semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
        :started_(false)
        ,joined_(false)
        ,tid_(0)
        ,func_(std::move(func))
        ,name_(name)
{
    setDeFaultName();
}
Thread::~Thread(){
    //join与detach不能同时成立，只能存在一个模式，要么阻塞，要么分离
    
    if(started_ && !joined_){
        //线程分离，
        thread_->detach();
    }
}
void Thread::start(){
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    //调用底层thread类，创建新线程
    thread_ = std::shared_ptr<std::thread>(new std::thread(
        [&](){//lambda表达式构造的线程函数
            tid_ = CurrentThread::tid();
            sem_post(&sem);
            //执行传递的线程函数
            func_();}
        ));
    //等待上面子线程执行完，获取到新的线程tid值
    sem_wait(&sem);

}

void Thread::join(){
    joined_ = true;
    thread_->join();
}
void Thread::setDeFaultName(){
    int num = ++numCreated_;
    if(name_.empty()){
       char buf[32]={0};
       snprintf(buf, sizeof buf, "Thread%d", num);
       name_ = buf; 
    }
}
