#ifndef TIMERQUEUE_H
#define TIMERQUEUE_H
#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "Timer.h"
#include <vector>
#include <set>
#include "Channel.h"
#include <memory>
#include "TimerId.h"
#include <atomic>
namespace mulib{
    namespace net{
        class EventLoop;
        class TimerQueue : noncopyable{
        public:
            TimerQueue(EventLoop *loop);
            ~TimerQueue();
            TimerId addTimer(const Timer::TimerCallback &cb, Timestamp when, double interval);
            void cancel(TimerId timerid);
        private:
            using Entry = std::pair<base::Timestamp, Timer *>;
            using TimerList = std::set<Entry>;
            using ActiveTimer = std::pair<Timer *, int64_t>;
            using ActiveTimerSet = std::set<ActiveTimer>;
            void addTimerInLoop(Timer *timer);
            void cancelInLoop(TimerId timerid);
            void handleRead();
            std::vector<Entry> getExpired(Timestamp now);
            void reset(const std::vector<Entry> &expired, Timestamp now);
            bool insert(Timer *timer);
            void resetTimerfd(int timerfd, Timestamp);
            EventLoop *loop_;
            const int timerfd_;
            Channel timerfdChannel_;
            TimerList timers_;
            ActiveTimerSet activeTimers_;
            bool callingExpiredTimers_;
            ActiveTimerSet cancelingTimers_; 
            int createTimerfd();
        };
    }
}
#endif
