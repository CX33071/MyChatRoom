#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H


#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include <functional>
#include <atomic>

namespace mulib{
    using base::Timestamp;
    namespace net{
        class Timer : noncopyable
        {
        public:
            using TimerCallback = std::function<void()>;
            Timer(TimerCallback cb, Timestamp when, double interval);
            void run() const;
            Timestamp expiration() const;
            bool repeat() const;
            int64_t sequence() const;

            void restart(Timestamp now);
            static int64_t numCreated();

        private:
            const TimerCallback callback_; // 定时器触发时要调用的回调函数
            Timestamp expiration_;         // 当前这次触发的时间点
            const double interval_;        // 表示定时器的触发间隔，单位为秒。
            const bool repeat_;            // 是否是周期性定时器

            const int64_t sequence_; // 每创建一个 Timer，这个号就会递增
            static std::atomic<int64_t> s_numCreated;
        };
    }
}
#endif//
