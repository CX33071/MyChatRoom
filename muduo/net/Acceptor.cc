#include "Acceptor.h"
#include "InetAddress.h"
using namespace mulib::net;
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr)
: loop_(loop) ,acceptSocket_(socket::createNonblockingOrDie()) ,
acceptChannel_(loop,acceptSocket_.fd()), listening_(false){
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback([this](Timestamp)
                                   { handleRead(); }); // 有“可读事件”发生时（也就是有新连接进来了）
}
void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}
void Acceptor::setNewConnectionCallback(const NewConnectionCallback &cb){
    newConnectionCallback_ = cb;
}
Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
}
void Acceptor::handleRead(){
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    int sock = acceptSocket_.accept(&peerAddr);
    if(sock >= 0){
        if (newConnectionCallback_){
            newConnectionCallback_(sock, peerAddr);
        }
        else{
            socket::close(sock);
        }
    }//
}
