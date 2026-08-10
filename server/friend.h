
#pragma once
#include <curl/curl.h>
#include <mysql/mysql.h>
#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <cpp_redis/cpp_redis>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <future>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include "Mysql.h"
struct friendchatrecord {
    std::string sender;
    std::string reciver;
    std::string content;
};
struct filestatusserver{
    std::string sended;
    std::string total;
    std::string filename;
    std::string from;
    std::string recived;
    std::string id;
};
bool sendn(int fd, char* data, ssize_t len);
class Friend {
   private:
    cpp_redis::client redis_;
    Mysql mysql_;

   public:
    Friend();
    bool addapply(std::string applyaccount, std::string appliedaccount) ;
    bool agreeapply(std::string applyaccount, std::string appliedaccount) ;
    bool refuseapply(std::string applyaccount, std::string appliedaccount) ;
    bool block(std::string applyaccount, std::string appliedaccount) ;
    int cancleblock(std::string applyaccount, std::string appliedaccount) ;
    int delfriend(std::string applyaccount, std::string appliedaccount) ;
    int isfriend(std::string account, std::string friendaccount) ;
    void delfriend1(std::string account, std::string target);
    int isblock(std::string applyaccount, std::string appliedaccount);
    std::string getname(std::string account);
    void addfriend(std::string account, std::string target);
    void historyfriendchat(std::string account1,
                           std::string account2,std::string sender,std::string reciver,
                           std::string content);
    std::vector<friendchatrecord> gethistoryfriendchat(std::string account1,
                              std::string account2);
    std::vector<std::string> friendlist(std::string account);
    std::vector<std::string> blocklist(std::string account);
    std::vector<std::string> onlinelist(std::string account);
};
