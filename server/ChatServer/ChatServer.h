#pragma once
#include <assert.h>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include "muduo/base/logger.h"
#include "muduo/net/TcpServer.h"
#include "Service/FILEredis.h"
#include "Service/account.h"
#include "Service/friend.h"
#include "Service/group.h"
using json = nlohmann::json;
class ChatServer {
   private:
    std::mutex g_mutex;
    std::map<std::string, TcpConnectionPtr> clientmap;
    Verifycode verifycode;
    Friend F;
    Group G;
    FILEredis file;
    std::unordered_map<std::string, std::unordered_set<std::string>> group_map;
    TcpServer server_;
    std::unordered_map<std::string, Timestamp> clientconntime;
    EventLoop* loop_;
    Timestamp T;

   public:
    ChatServer(EventLoop* loop, std::string name, const InetAddress& addr);
    void connectioncallback(const TcpConnectionPtr& conn);
    std::string get_current_time();
    void getrequest_id(json j, json& j1);
    void messagecallback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp);
    void checkoutclientconn();
    void sendtotarget(std::string account, json j);
};
