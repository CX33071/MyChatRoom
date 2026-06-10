#include <assert.h>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
#include "friend.h"
#include "account.h"
std::mutex g_mutex;
std::map<std::string, TcpConnectionPtr> clientmap;
Verifycode verifycode;
Friend F;
void connectioncallback(const TcpConnectionPtr& conn) {
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
    size_t pos = message.find(':');
    std::string cmd = message.substr(0, pos);
    std::string data = message.substr(pos + 1);
   if(cmd=="signup"){
       size_t pos = data.find(' ');
       std::string account = data.substr(0, pos);
       std::string password = data.substr(pos + 1);
       bool res=verifycode.signup(account,password);
       if(res){
           conn->send("注册成功");
       }else{
           conn->send("用户帐号已经存在，请重新输入账号");
       }
   }
   if(cmd=="verifycode:"){
       verifycode.addredis(data);
   }
   if(cmd=="verifycodesignin:"){
       size_t pos = data.find(' ');
       std::string account = data.substr(0, pos);
       std::string code = data.substr(pos + 1);
       bool res = verifycode.verify(account, code);
       if(res){
           conn->send("验证码正确，登录成功");
       }else{
           conn->send("验证码错误，登录失败");
       }
   }
        if(cmd=="keysignin:"){
            size_t pos = data.find(' ');
            std::string account = data.substr(0, pos);
            std::string password = data.substr(pos + 1);
            bool res=verifycode.loginwithkey(account,password);
        if(res){
        conn->send("密码正确，登录成功!");
        }else{
        conn->send("密码错误，登录失败");
        }
        }
        if(cmd=="forgetkey:"){
           bool res= verifycode.forgetkey(data);
           if(res){
               conn->send("该账号并未注册");
           }else{
               conn->send("密码已经发到您的邮箱");
           }
        }
    if(cmd=="destory:"){
        size_t pos = data.find(' ');
        std::string account = data.substr(0, pos);
        std::string password = data.substr(pos + 1);
        bool res=verifycode.destroy(account,password);
        if(res){
            conn->send("密码错误，请重新输入");
        }else{
            conn->send("密码正确，注销成功");
        }
    }
    if(cmd=="addfriend:"){
        size_t pos = data.find(' ');
        std::string account = data.substr(0, pos);
        std::string friendaccount = data.substr(pos + 1);
        bool res=F.addapply(account, friendaccount);
        if(res){
            conn->send("好友申请已发送");
            auto it = clientmap.find(friendaccount);
            if (it != clientmap.end()) {
                it->second->send("有一条来自" + account + "的好友申请");
            }
        }else{
            conn->send("要添加的好友账号不存在");
        }
        }
        if(cmd=="agreefriend:"){
            size_t pos=data.find(' ');
            std::string account = data.substr(0, pos);
            std::string friendaccount = data.substr(pos + 1);
            F.agreeapply(account, friendaccount);
            conn->send("已同意对方的好友申请");
            auto it = clientmap.find(friendaccount);
            if(it!=clientmap.end()){
                it->second->send(friendaccount + "已经同意你的好友申请");
            }
        }
        if(cmd=="refusefriend"){
            size_t pos = data.find(' ');
            std::string account = data.substr(0, pos);
            std::string friendaccount = data.substr(pos + 1);
            F.refuseapply(account, friendaccount);
            conn->send("");
            auto it = clientmap.find(friendaccount);
            if (it != clientmap.end()) {
                it->second->send(friendaccount + "已经同意你的好友申请");
            }
        }
    if(cmd=="friendlist:"){
        std::vector<std::string> res = F.friendlist(data);
        std::string list="好友列表:\n";
        for (int i = 0; i < res.size();i++){
            list += res[i];
            list += "\n";
        }
        conn->send(list);
    }
    if(cmd=="chat:"){
        size_t pos = data.find(' ');
        std::string account = data.substr(0, pos);
        size_t p=data.find(':');
        std::string friendaccount = data.substr(pos + 1,p);
        std::string content = data.substr(p + 1);
        auto it = clientmap.find(friendaccount);
        if(it!=clientmap.end()){
            it->second->send(content);
        }
    }
    if(cmd=="block:"){
        size_t pos = data.find(' ');
        std::string account = data.substr(0, pos);
        std::string blockedaccount=data.substr(pos+1);
        F.block(account, blockedaccount);
        conn->send("该用户已经拉黑");
    }
    if(cmd=="delfriend:"){
        size_t pos = data.find(' ');
        std::string account = data.substr(0, pos);
        std::string delfriend = data.substr(pos + 1);
        F.delfriend(account,delfriend);
        conn->send("该好友已经删除");
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