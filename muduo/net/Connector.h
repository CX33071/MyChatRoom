#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H
#include "../base/noncopyable.h"
#include "InetAddress.h"
#include <functional>
#include <memory>
namespace mulib{
    namespace net{//
        class Channel;
        class EventLoop;
        class Connector : noncopyable,public std::enable_shared_from_this<Connector>{
        public:
            using NewConnectionCallback = std::function<void(int sockfd)>;
            Connector(EventLoop *loop, const InetAddress &serverAddr);
            ~Connector();
            void setNewConnectionCallback(const NewConnectionCallback &cb){newConnectionCallback_ = cb;}
            void start();
            void restart();
            void stop();
            const InetAddress &serverAddress() const { return serverAddr_; }
        private:
            enum States
            {
                kDisconnected,
                kConnecting,
                kConnected
            };
            static const int kMaxRetryDelayMs = 30 * 1000;
            static const int kInitRetryDelayMs = 500;
            void setState(States s) { state_ = s; }
            void startInLoop();
            void stopInLoop();
            void connect();
            void connecting(int sockfd);
            void handleWrite();
            void handleError();
            void retry(int sockfd);
            int removeAndResetChannel();
            void resetChannel();
            EventLoop *loop_;  // 客户端所在的事件循环
            InetAddress serverAddr_;  // 服务器地址
            bool connect_; // 是否正在连接
            States state_;   // 连接状态
            std::unique_ptr<Channel> channel_;  // 指向客户端的channel
            NewConnectionCallback newConnectionCallback_; // 连接成功后的回调
            int retryDelayMs_;    // 重试的延迟时间
        };
    }
}
#endif
