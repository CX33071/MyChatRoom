#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <thread>
#include <mutex>
#include "../base/noncopyable.h"
#include "TimerId.h"
#include "TimerQueue.h"
#include "sigpipe.h"
#include <memory>
#include <vector>
#include <atomic>

namespace mulib{
    namespace net{
        class Epoller;
        class Channel;
        class EventLoop : noncopyable{
        public:
            EventLoop();//
            ~EventLoop();

            void loop(int timeout);
            void quit();

            void assertInLoopThread();
            bool isInLoopThread() const;

            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);

            TimerId runAt(const Timestamp &time, const Timer::TimerCallback &cb);
            TimerId runAfter(double delay, const Timer::TimerCallback &cb);
            TimerId runEvery(double interval, const Timer::TimerCallback &cb);
            void cancel(TimerId id);

            using Functor = std::function<void()>;
            void runInLoop(const Functor &cb);
            void queueInLoop(const Functor &cb);
            void wakeup();
            int createEventfd();

        private:
            void abortNotInLoopThread();
            void handleRead();
            void doPendingFunctors();

            using ChannelList = std::vector<Channel *>;

            bool looping_;          // 是否处于 loop() 状态（是否已经在事件循环中）
            std::atomic<bool> quit_; // 是否退出循环，线程安全
            int64_t iteration_;      // 循环次数，调试或统计用
            Timestamp pollReturnTime_; // 每轮 poll 返回时间戳，用于定时器判断等
            const std::thread::id threadId_; // 创建该 EventLoop 的线程 id，用于线程检查

            std::unique_ptr<Epoller> poller_;
            ChannelList activeChannels_; // 本轮 epoll 触发的 Channel 列表

            // std::unique_ptr<TimerQueue> timerQueue_;
            //管理定时器的类（内部使用 timerfd + 最小堆）
            std::vector<Functor> pendingFunctors_; // 延迟执行的任务队列
            bool callingPendingFunctors_;          // 是否正在执行 pendingFunctors_，防止嵌套调用
            std::mutex mutex_;                     // 保护 pendingFunctors_ 的互斥锁

            int wakeupFd_;
            std::unique_ptr<Channel> wakeupChannel_;
            std::unique_ptr<TimerQueue> timerQueue_;
        };
    }
}
#endif
