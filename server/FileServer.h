#pragma once

#include <assert.h>
#include <cpp_redis/cpp_redis>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
#include "FILEredis.h"
#include "json.hpp"
#include "Mysql.h"
using json = nlohmann::json;
class FileServer{
    private:
    cpp_redis::client redis_;
    Mysql mysql_;
    std::atomic<bool> groupsend = false;
    TcpServer server_;
    enum class FileState { PRASEJSON, RECV_FILE, SEND_FILE };
    struct FileContext {
        FileState state = FileState::PRASEJSON;
        int fd = -1;
        std::string filename;
        uint64_t filesize = 0;
        uint64_t recvsize = 0;
        std::string ID;
    };

     std::unordered_map<TcpConnectionPtr, FileContext> uploadmap_;

    public:
     FileServer(EventLoop* loop, std::string name, const InetAddress& addr);
     void connectioncallback(const TcpConnectionPtr& conn);
     void messagecallback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp);
};