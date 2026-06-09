#include "/home/cx33071/muduo-/net/TcpClient.h"
#include <condition_variable>
TcpClient::TcpConnectionPtr g_conn;
std::condition_variable g_cv;
std::mutex g_mutex;
bool connok = false;
void connectioncallback(const TcpClient::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO << "成功连接服务器";
        g_conn = conn;
        connok = true;
        g_cv.notify_all();
    } else {
        LOG_INFO << "与服务器断开连接";
        g_conn.reset();
        connok = false;
    }
}
void messagecallback(const TcpClient::TcpConnectionPtr&conn,Buffer*buf,Timestamp){
    std::string message = buf->retrieveAllAsString();
    std::cout << "收到消息" << message << std::endl;
}
void input(){
    std::unique_lock<std::mutex> lock(g_mutex);
    g_cv.wait(lock, [] { return connok; });
    lock.unlock();
    std::string line;
    while(std::getline(std::cin,line)){
        g_conn->send(line);
    }
}
int main(int argc,char*argv[]){
    Logger logger;
    logger.setLogLevel(Logger::INFO);
    EventLoop loop;
    InetAddress addr(argv[1], 8888);
    TcpClient client(&loop, addr);
    client.setConnectionCallback(connectioncallback);
    client.setMessageCallback(messagecallback);
    client.connect();
    std::thread t(input);
    t.detach();
    int timeout;
    loop.loop(timeout);
    return 0;
}