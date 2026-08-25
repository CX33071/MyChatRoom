#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>

namespace mulib{
    namespace net{
        class EventLoopThread
        {
        public:
            EventLoopThread() : loop_(nullptr), exiting_(false) {}
            ~EventLoopThread();
            EventLoop *startLoop();

        private:
            void threadFunc();
            std::thread thread_;
            EventLoop *loop_;
            std::mutex mutex_;
            std::condition_variable cond_;
            bool exiting_;
        };
    }
}
//
using namespace mulib::net;
inline EventLoop *EventLoopThread::startLoop()
{
    thread_ = std::thread([this]
                          { threadFunc(); });

    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]()
                   { return loop_ != nullptr; });
    }

    return loop_;
}
inline EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
    }
    thread_.join();
}
inline void EventLoopThread::threadFunc(){
    EventLoop loop;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop(-1);
}
#endif
