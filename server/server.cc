
#pragma once
#include <assert.h>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
#include "friend.h"
#include "json.hpp"
#include "group.h"
#include <unordered_map>
#include <unordered_set>
#include "ChatServer.h"
#include "FileServer.h"
#include "FILEredis.h"
using json = nlohmann::json;
int main(){
    Logger logger;
    logger.setLogLevel(Logger::INFO);
    EventLoop loop;
    InetAddress chataddr(8888);
    InetAddress fileaddr(9999);
    ChatServer chatserver(&loop, "chatserver", chataddr);
    FileServer fileserver(&loop, "fileserver", fileaddr);
    int timeout = -1;
    loop.loop(timeout);
    return 0;
}