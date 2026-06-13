#include <assert.h>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
#include "friend.h"
#include "json.hpp"
#include "group.h"
#include <unordered_map>
#include <unordered_set>
using json = nlohmann::json;
std::mutex g_mutex;
std::map<std::string, TcpConnectionPtr> clientmap;
Verifycode verifycode;
Friend F;
Group G;
std::unordered_map<std::string, std::unordered_set<std::string>> group_map;
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
    json j;
    j = json::parse(message);
    std::string cmd=j["cmd"];
    if(cmd=="signup"){
    std::string account=j["account"];
    std::string password = j["password"];
    bool res = verifycode.signup(account, password);
    json j1;
    j1["cmd"] = "signup_res";
    if (res) {
        j1["data"] = "成功登录";
    } else {
        j1["data"] = "该账号已被注册过，请重新尝试";
    }
    conn->send(j1.dump());
    }
    if(cmd=="verifycode"){
        std::string account = j["account"];
        verifycode.addredis(account);
    }
    if(cmd=="verifycodesignin"){
        std::string account = j["account"];
        std::string code = j["code"];
        bool res = verifycode.verify(account, code);
        json j1;
        j1["cmd"] = "codesignin_res";
        if (res) {
            j1["code"] = "1";
            j1["data"] = "验证码正确，登录成功";
            clientmap[account] = conn;
        } else {
            j1["code"] = "2";
            j1["data"] = "验证码错误，登录失败";
        }
        conn->send(j1.dump());
    }
    if(cmd=="keysignin"){
        std::string account = j["account"];
        std::string password = j["password"];
        int res = verifycode.loginwithkey(account,password);
        json j1;
        j1["cmd"] = "keysignin_res";
        if (res==0) {
            j1["code"] = "1";
            j1["data"] = "密码正确，登录成功";
            clientmap[account] = conn;
        } else if (res == 2) {
            j1["code"] = "2";
            j1["data"] = "密码错误，登录失败";
        } else {
            j1["code"] = "2";
            j1["data"] = "该账号并不存在";
        }
        conn->send(j1.dump());
    }
    if(cmd=="forgetkey"){
        std::string account = j["account"];
        bool res = verifycode.forgetkey(account);
        json j1;
        j1["cmd"] = "forgetkey_res";
        if (res) {
            j1["data"] = "该账号并未注册";
        } else {
            j1["data"] = "密码已经发到您的邮箱";
        }
        conn->send(j1.dump());
    }
    if(cmd=="destory"){
        std::string account = j["account"];
        std::string password = j["password"];
        bool res = verifycode.destroy(account, password);
        json j1;
        j1["cmd"] = "destory_res";
        if (res) {
            j1["data"] = "注销账号成功";
        } else {
            j1["data"] = "密码错误，注销失败";
        }
        conn->send(j1.dump());
    }
    if(cmd=="addfriend"){
    std::string from=j["from"]; 
    std::string to = j["to"];
    bool res = F.addapply(from,to);
    json j1;
    j1["cmd"] = "addres";
    if (res) {
        j1["data"] = "好友申请已经发送";
        auto it = clientmap.find(to);
        json j2;
        std::string s = "有一条来自" + from + "的好友申请";
        j2["cmd"] = "addedres";
        j2["message"]=s;
        j2["target"] = to;
        if (it != clientmap.end()) {
            it->second->send(j2.dump());
        }
    } else {
        j1["data"] = "要添加的好友账号不存在";
    }
    conn->send(j1.dump());
    }
    if(cmd=="agreefriend"){
    std::string account=j["account"];
    std::string friendaccount = j["friendaccount"];
    F.agreeapply(account, friendaccount);
    json j1, j2;
    j1["cmd"] = "agreeres";
    j2["cmd"] = "agreedres";
    j1["data"] = "同意对方的好友申请";
    j2["data"] = account + "已经同意你的好友申请";
    conn->send(j1.dump());
    auto it = clientmap.find(friendaccount);
    if(it!=clientmap.end()){
    it->second->send(j2.dump());
}
    }
    if(cmd=="refusefriend"){
        std::string account = j["account"];
        std::string friendaccount = j["friendaccount"];
        F.refuseapply(account, friendaccount);
        json j1, j2;
        j1["cmd"] = "refuseres";
        j2["cmd"] = "refusedres";
        j1["data"] = "已经拒绝对方的好友申请";
        j2["data"] = account + "拒绝了你的好友申请";
        auto it = clientmap.find(friendaccount);
        if (it != clientmap.end()) {
            it->second->send(j2.dump());
        }
        conn->send(j1.dump());
    }
    if(cmd=="friendlist"){
        std::string account = j["account"];
        std::vector<std::string> res = F.friendlist(account);
        std::string list = "好友列表:\n";
        for (int i = 0; i < res.size(); i++) {
            list += res[i];
            list += "\n";
        }
        json j1;
        j1["cmd"] = "friendlistres";
        j1["data"] = list;
        conn->send(j1.dump());
    }
    if(cmd=="chat"){
std::string account= j["account"];
std::string target = j["target"];
std::string message = j["message"];
json j1, j2;
j1["cmd"] = "chatres";
j2["cmd"] = "chatedres";
j1["data"] = "已给" + target + "发送消息";
auto it = clientmap.find(target);
if(it!=clientmap.end()){
    j2["data"] = "收到来自" + account + "的消息:" + message;
    it->second->send(j2.dump());
    conn->send(j1.dump());
} else {
    j1["data"] = "对方当前不在线";
    conn->send(j1.dump());
}
    }
    if(cmd=="block"){
std::string account=j["account"];
std::string target=j["target"];
bool res=F.block(account,target);
json j1;
j1["cmd"] = "blockres";
if (res) {
    j1["data"] = "已经拉黑" + target;
} else {
    j1["data"] = "拉黑失败，该用户不存在";
}
conn->send(j1.dump());
    }
    if(cmd=="cancleblock"){
        std::string account=j["cancleaccount"];
        std::string target=j["target"];
        int res = F.cancleblock(account, target);
        json j1;
        j1["cmd"] = "cancleres";
        if (res == 0) {
            j1["data"] = "已成功取消拉黑";
        } else if (res == 1) {
            j1["data"] = "目标用户不存在";
        } else {
            j1["data"] = "目标用户本身并不在拉黑名单";
        }
        conn->send(j1.dump());
    }
    if(cmd=="delfriend"){
    std::string account=j["account"];
    std::string target=j["target"];
    int res=F.delfriend(account,target);
    json j1;
    j1["cmd"] = "delfriendres";
    if (res == 1) {
        j1["data"] = "目标用户根本不存在";
    } else if (res == 2) {
        j1["data"] = "目标用户还不是好友";
    } else {
        j1["data"] = "已删除目标用户好友";
    }
    conn->send(j1.dump());
    }
    if(cmd=="creategroup"){
    std::string groupname=j["groupname"];
    std::string account=j["account"];
    std::string res = G.creategroup(account,groupname);
    json j1;
    j1["cmd"] = "creategroupres";
    j1["data"] = "群聊已成功创建:"+res;
    conn->send(j1.dump());
    }
    if(cmd=="invite"){
        std::string groupname=j["groupname"];
        std::string account=j["account"];
        std::string target=j["target"];
        int res=G.invite(account,target,groupname);
        json j1, j2;
        j1["cmd"] = "inviteres";
        if(res==0){
        j1["data"] = "邀请入群消息已经发送";
            auto it = clientmap.find(target);
            json j2;
            std::string s = "有一条来自" + account + "加入群:"+groupname+"的申请";
            j2["cmd"] = "invitedres";
            j2["data"] = s;
            j2["groupname"] = groupname;
            j2["account"] = account;
            if (it != clientmap.end()) {
                it->second->send(j2.dump());
            }
        }else if(res==1){
        j1["data"] = "要邀请的好友账号不存在";
        }else{
            j1["data"] = "要邀请进群的好友已经在群里！";
        }
        conn->send(j1.dump());
    }
    if(cmd=="agreejoin"){
        std::string account = j["account"];
        std::string groupname = j["groupname"];
        G.agreejoin(account, groupname);
        json j1, j2;
        j1["cmd"] = "agreegroupres";
        j2["cmd"] = "agreedgroupres";
        j1["data"] = "同意对方的邀请入群申请";
        j2["data"] = account + "已经同意你的邀请入群申请";
        conn->send(j1.dump());
        auto it = clientmap.find(account);
        if (it != clientmap.end()) {
            it->second->send(j2.dump());
        }
    }
    if(cmd=="refusegroup"){
        std::string account = j["account"];
        std::string target=j["target"];
        std::string groupname = j["groupname"];
        G.refusejoin(account,target,groupname);
        json j1, j2;
        j1["cmd"]="refusegroupres";
        j2["cmd"] = "refusedgroupres";
        j1["data"] = "已拒绝对方的入群邀请";
        j2["data"]=account+"拒绝了你的邀请入群申请";
        conn->send(j1.dump());
        auto it=clientmap.find(target);
        if(it!=clientmap.end()){
            it->second->send(j2.dump());
        }
    }
    if(cmd=="delgroup"){
std::string account=j["account"];
std::string groupname = j["groupname"];
int res = G.delgroup(groupname, account);
json j1;
j1["cmd"] = "delgroupres";
if (res==0) {
    j1["data"] = "删除群聊成功";
}else if(res==1){
    j1["data"] = "该群聊并不存在";
}else{
    j1["data"] = "你不是该群群主，没有解散群聊的权限!";
}
conn->send(j1.dump());
    }
if (cmd == "groupchat") {
    std::string account=j["account"];
    std::string msg = j["groupmsg"];
    std::string groupname = j["groupname"];
    json j1;
    j1["cmd"] = "groupchatres";
    auto it = group_map.find(groupname);
    std::string s =
        "收到来自" + groupname + "的成员:" + account + "发来的消息:" + msg;
    if (it == group_map.end()) {
        j1["data"] = "该群并不存在";
    } else {
        bool res = G.groupchat(groupname, account, msg);
        j1["data"] = "消息已发送";
        for (auto it1 : it->second) {
            auto it2 = clientmap.find(it1);
            if(it2!=clientmap.end()){
                json j2{{"cmd", "groupchatedres"}, {"data", s}};
                it2->second->send(j2.dump());
            }
        }
    }
    conn->send(j1.dump());
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
    int timeout=-1;
    loop.loop(timeout);
    return 0;
}