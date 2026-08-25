#include "Connector.h"
#include "../base/logger.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketOps.h"
#include <assert.h>
using namespace mulib::net;

Connector::Connector(EventLoop*loop,const InetAddress&serverAddr) : loop_(loop),
serverAddr_(serverAddr),
connect_(false),
state_(kDisconnected),
retryDelayMs_(kInitRetryDelayMs){
    LOG_DEBUG << "ctor[" << this << "]";
}
Connector::~Connector(){
    LOG_DEBUG << "dtor[" << this << "]";
    assert(!channel_);
}
void Connector::start(){
    connect_ = true;
    loop_->runInLoop([this]
                     { startInLoop(); });
}
void Connector::startInLoop(){
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_){
        connect();
    }
    else{
        LOG_DEBUG << "do not connect";
    }
}
void Connector::stop(){
    connect_ = false;
    loop_->queueInLoop([this]
                       { stopInLoop(); });
}
void Connector::stopInLoop(){
    loop_->assertInLoopThread();
    if (state_ == kConnecting){
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}
void Connector::connect(){
    int sockfd = socket::createNonblockingOrDie();
    int ret = socket::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;
    switch (savedErrno)
    {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
        connecting(sockfd);
        break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        retry(sockfd);
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
        socket::close(sockfd);
        break;

    default:
        LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
        socket::close(sockfd);
        // connectErrorCallback_();
        break;
    }
}
void Connector::restart(){
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}
void Connector::connecting(int sockfd){
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback([this] { Connector::handleWrite(); });
    channel_->setErrorCallback([this] { Connector::handleError(); });
    channel_->enableWriting();
}
int Connector::removeAndResetChannel(){
    channel_->disableAll();
    int sockfd = channel_->fd();
    loop_->queueInLoop([this] { Connector::resetChannel(); });
    return sockfd;
}
void Connector::resetChannel(){
    channel_.reset();
}
void Connector::handleError(){
    LOG_ERROR << "Connector::handleError state=" << state_;
    if(state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror(err);
        retry(sockfd);
    }
}
void Connector::handleWrite(){
    LOG_TRACE << "Connector::handleWrite " << state_;
    if(state_ == kConnecting){
        int sockfd = removeAndResetChannel();
        int err = socket::getSocketError(sockfd);
        if (err){
            LOG_WARN << "Connector::handleWrite - SO_ERROR = "
                     << err << " " << strerror(err);
            retry(sockfd);
        }
        else if(socket::isSelfConnect(sockfd)){
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else{
            setState(kConnected);
            if(connect_){
                newConnectionCallback_(sockfd);
            }
            else{
                socket::close(sockfd);
            }
        }
    }
    else{
        assert(state_ == kDisconnected);
    }
}
const int Connector::kMaxRetryDelayMs;
void Connector::retry(int sockfd){
    socket::close(sockfd);
    setState(kDisconnected);
    if(connect_){
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toHostPort() << " in " << retryDelayMs_ << " milliseconds. ";
        loop_->runAfter(retryDelayMs_ / 1000.0, [this]
                        { startInLoop(); });
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    }
    else{
        LOG_DEBUG << "do not connect";
    }
}
