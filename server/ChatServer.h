#pragma once
#include <assert.h>
#include <glog/logging.h>
#include <unordered_map>
#include <unordered_set>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
#include "FILEredis.h"
#include "account.h"
#include "friend.h"
#include "group.h"
#include "json.hpp"
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
    std::atomic<long long> fileid{0};
    TcpServer server_;
    std::unordered_map<std::string, Timestamp> clientconntime;
    EventLoop* loop_;
    Timestamp T;

   public:
    ChatServer(EventLoop* loop, std::string name, const InetAddress& addr);
    void connectioncallback(const TcpConnectionPtr& conn);
    std::string get_current_time();
    void messagecallback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp);
    void checkoutclientconn();
};
