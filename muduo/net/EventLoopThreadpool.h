#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H
#include "EventLoopThread.h"
#include "../base/noncopyable.h"
namespace mulib{
    namespace net{
        class EventLoopThreadPool : noncopyable{
        public:
            EventLoopThreadPool(EventLoop *baseloop);
            ~EventLoopThreadPool();
            void setThreadNum(int numThreads) { numThreads_ = numThreads; }
            void start();
            EventLoop *getNextLoop();

        private:
            EventLoop *baseLoop_;
            bool started_;
            int numThreads_;
            int next_;

            std::vector<std::shared_ptr<EventLoopThread>> threads_;
            std::vector<EventLoop *> loops_;
        };
        
    }
}
#endif
