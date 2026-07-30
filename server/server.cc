
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
#include <glog/logging.h>
#define RESET "\033[0m"
#define GREEN "\033[1;32m"
#define BLUEC "\033[1;34m"
#define BLUE "\033[34m"
#define PURPLE "\033[1;35m"
using json = nlohmann::json;
int isport(char*s){
    char* end;
    int port = std::strtol(s, &end, 10);
    if(*end!='\0'){
        return 1;
    }
    if(port<=0||port>65535){
        return 2;
    }
    return 0;
}
int main(int argc,char*argv[]){
    if(argc!=4){
        std::cout << PURPLE << "请输入./server (ip) (chat_port) (file_port)" << RESET
                  << std::endl;
        return -1;
    }
    int res1 = isport(argv[2]);
    int res2 = isport(argv[3]);
    if(res1==1||res2==1){
        std::cout << PURPLE << "port请输入数字!" << RESET << std::endl;
        return -1;
    }
    if(res1==2||res2==2){
        std::cout << PURPLE << "port范围为0~65535!" << RESET << std::endl;
        return -1;
    }
    google::InitGoogleLogging(argv[0]);
    FLAGS_log_dir = "./GLog";
    LOG(INFO) << "ChatServer Start!";
    LOG(INFO) << "FileServer Start!";
    Logger logger;
    logger.setLogLevel(Logger::INFO);
    EventLoop loop;
    InetAddress chataddr(argv[1],std::stoi(argv[2]));
    InetAddress fileaddr(argv[1],std::stoi(argv[3]));
    ChatServer chatserver(&loop, "chatserver", chataddr);
    FileServer fileserver(&loop, "fileserver", fileaddr);
    int timeout = -1;
    loop.loop(timeout);
    google::ShutdownGoogleLogging();
    return 0;
}