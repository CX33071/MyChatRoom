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
#include "muduo/base/logger.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "database/Mysql.h"
#include "friend.h"
class Group{
    private:
     cpp_redis::client redis_;
     Mysql mysql_;
     std::unordered_map<std::string, std::unordered_set<std::string>> group_map;
    public:
     Group();
     std::string creategroup(std::string account, std::string name);
     int invite(std::string account,
                 std::string invaccount,
                 std :: string name);
     int delgroup(std::string groupname, std::string account);
     bool agreejoin(std::string account, std::string groupname);
     bool refusejoin(std::string account,
                     std::string groupname);
    std::vector<std::string> applyjoingroup(std::string account,std::string groupname);
    std::string findowner(std::string groupname);
    std::string getname(std::string account);
    std::vector<std::string> grouplist(std::string account);
    std::vector<std::string> ownergrouplist(std::string account);
    int exitgroup(std::string account, std::string groupname);
    std::vector<std::string> groupmembers(std::string account,
                                          std::string groupname);
    int is_groupmember(std::string groupname, std::string account);
    int is_existsgroup(std::string groupname);
    int is_manager(std::string groupname, std::string account);
    void addmanager(std::string groupname, std::string account);
    void delmanager(std::string groupname, std::string account);
    void changeowner(std::string groupname,
                     std::string owner,
                     std::string account);
    void delmember(std::string groupname, std::string account);
    std::vector<std::string> grouptargetmember(std::string groupname,std::string account);
    void historygroupchat(std::string account,
                          std::string groupname,std::string name,
                          std::string content);
    std::vector<friendchatrecord> getgrouphistory(std::string groupname);
    void disconnectmsg(std::string account, json j);
    std::vector<std::string> getdisconnectmsg(std::string account);
    bool is_owner(std::string account, std::string groupname);
    void destorydismsg(std::string account);
};