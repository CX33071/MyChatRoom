#include "TimerQueue.h"
#include "EventLoop.h"
#include "TimerId.h"
#include <algorithm>
#include "../base/Timestamp.h"
#include "../base/logger.h"
#include <sys/timerfd.h>
#include <assert.h>
#include <unistd.h>
#include <memory.h>

using namespace mulib::net;

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_() {}

int TimerQueue::createTimerfd(){
    return timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
}
TimerId TimerQueue::addTimer(const Timer::TimerCallback &cb, Timestamp when, double interval){
    Timer *timer = new Timer(cb, when, interval);
    loop_->runInLoop([this, timer]
                     { addTimerInLoop(timer); 
                    });
    return TimerId(timer,timer->sequence());
}
void TimerQueue::cancel(TimerId timerid){
    loop_->runInLoop([this, timerid]
                     { cancelInLoop(timerid); });
}

bool TimerQueue::insert(Timer *timer){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if(it == timers_.end() || when < it->first){
        earliestChanged = true; // 如果当前定时器是第一个插入的，或者它的触发时间比原来最早的还要早，那么我们需要更新 timerfd 的到期时间（所以设 earliestChanged = true）。
    }
    {
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);// set只能插入没有的变量
        (void)result; // 显式声明我知道这个变量没用，但我故意写它为了避免编译器报 “变量未使用” 的警告
    }
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}
namespace mulib{
    namespace net{
        struct timespec howMuchTimeFromNow(Timestamp when){
            int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
            if(microseconds < 100){
            microseconds = 100;
            }
            timespec ts;
            ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
            ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
            return ts;
        }
    }
}

void TimerQueue::resetTimerfd(int timerfd, Timestamp expiration)
{
    itimerspec newValue;
    itimerspec oldValue;
    bzero(&newValue, sizeof(newValue));
    bzero(&oldValue, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if(ret){
        LOG_SYSERR << "timerfd_settime()";
    }
}
void TimerQueue::addTimerInLoop(Timer *timer){
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);
    if(earliestChanged){
        resetTimerfd(timerfd_, timer->expiration());
    }
}
void TimerQueue::cancelInLoop(TimerId timerid){
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerid.timer_, timerid.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if(it != activeTimers_.end()){
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1);
        (void)n;
        
        activeTimers_.erase(it);// 反顺序
        delete it->first;
    }else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}
void TimerQueue::handleRead(){
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for(auto& it : expired){
        it.second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}
std::vector<mulib::net::TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    assert(timers_.size() == activeTimers_.size());
    std::vector<mulib::net::TimerQueue::Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);

    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now){
    Timestamp nextExpire;
    for(auto& it : expired){
        ActiveTimer timer(it.second, it.second->sequence());
        if(it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()){
            it.second->restart(now);
            insert(it.second);
        }
        else{
            delete it.second;
        }    
    }
    if(!timers_.empty()){
        nextExpire = timers_.begin()->second->expiration();
    }
    if (nextExpire.valid())
    {
        resetTimerfd(timerfd_, nextExpire);
    }
}
TimerQueue::~TimerQueue(){
    ::close(timerfd_);
    for (const Entry &timer : timers_)
    {
        delete timer.second;
    }
}
