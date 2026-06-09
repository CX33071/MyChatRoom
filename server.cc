#include <assert.h>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
std::mutex g_mutex;
std::map<std::string, TcpConnectionPtr> clientmap;
void connectioncallback(const TcpConnectionPtr&conn){
    if(conn->connected()){
        LOG_INFO << "有一个客户端连接成功:" << conn->peerAddress().toIpPort();
    }else{
        LOG_INFO << "有一个客户端下线了:" << conn->peerAddress().toIpPort();
        for (auto it = clientmap.begin(); it != clientmap.end();++it){
            if(it->second==conn){
                clientmap.erase(it);
                break;
            }
        }
    }
}
void messagecallback(const TcpConnectionPtr&conn,Buffer*buf,Timestamp){
    std::string message = buf->retrieveAllAsString();
    // LOG_INFO << "收到来自" << conn->localAddress().toIpPort() << "的消息："
    //          << conn->peerAddress().toIpPort();
    size_t pos = message.find(':');
    if (pos == std::string::npos) {
        conn->send("系统：格式错误，请使用：目标用户：消息");
        return;
    }
    if (message.substr(0, 6) == "login:") {
        std::string username = message.substr(6);
        clientmap[username] = conn;
        LOG_INFO << "用户登录:" << username;
        conn->send("系统：登录成功");
        return;
    }
        std::string target = message.substr(0, pos);
    std::string message1 = message.substr(pos + 1);
    auto it=clientmap.find(target);
    if(it!=clientmap.end()){
        it->second->send(message1);
    }else{
        conn->send("系统：目标用户不在线");
    }
}
int main(){
    Logger logger;
    logger.setLogLevel(Logger::INFO);
    EventLoop loop;
    InetAddress addr(8888);
    TcpServer server(&loop,"server", addr);
    server.setThreadNum(4);
    server.setMessageCallback(messagecallback);
    server.setConnectionCallback(connectioncallback);
    LOG_INFO << "服务器启动";
    server.start();
    int timeout;
    loop.loop(timeout);
    return 0;
}