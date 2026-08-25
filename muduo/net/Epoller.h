#ifndef MUDUO_NET_EPOLLPOLLER_H
#define MUDUO_NET_EPOLLPOLLER_H
#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "../base/logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include <vector>
#include <sys/epoll.h>
#include <map>
namespace mulib{
    namespace net{
        class Channel;
        class EventLoop;
        class Epoller : noncopyable{
        public:
            using ChannelList = std::vector<Channel *>;
            Epoller(EventLoop *loop);
            ~Epoller();
            base::Timestamp poll(int timeoutMs, ChannelList &activeChannels);

            void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }
            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);
        private:
            static const int kInitEventListSize = 16;
            void fillActiveChannels(int numEvents, ChannelList &activeChannels) const;
            void update(int opt, Channel *channel);
            using EventList = std::vector<struct epoll_event>;
            using ChannelMap = std::map<int, Channel *>;
            EventLoop *ownerLoop_;
            int epollfd_;
            EventList events_;
            ChannelMap channels_;
            
        };
    }
}
#endif//

