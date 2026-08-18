#pragma once

#include <curl/curl.h>
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
#include <unordered_map>
#include <unordered_set>
#include "database/Mysql.h"
#include "friend.h"
using json = nlohmann::json;
class FILEredis {
   private:
    Mysql mysql_;

   public:
    FILEredis();
    std::string friendupbegin(std::string sender,
                              std::string reciver,
                              std::string filename,
                              std::string filesize,
                              std::string ischat,
                              std::string uppath);
    std::string groupupbegin(std::string sender,
                             std::string groupname,
                             std::string filename,
                             std::string filesize,
                             std::string ischat,
                             std::string uppath);
    std::string getsender(std::string fileid);
    std::string getreciver(std::string fileid);
    std::string getgroupname(std::string fileid);
    std::string getfilesize(std::string fileid);
    std::string getdownpath(std::string fileid);
    std::string getuppath(std::string fileid);
    std::string getupsize(std::string fileid);
    std::string getdownsize(std::string fileid);
    std::string getfilename(std::string fileid);
    std::vector<filestatusserver> getfriendupinglist(std::string account);
    std::vector<filestatusserver> getchatfriendupinglist(std::string account);
    std::vector<filestatusserver> getgroupupinglist(std::string account);
    std::vector<filestatusserver> getchatgroupupinglist(std::string account);
    std::vector<filestatusserver> getfrienddowninglist(std::string account);
    std::vector<filestatusserver> getchatfrienddowninglist(std::string account);
    std::vector<filestatusserver> getgroupdowninglist(std::string account);
    std::vector<filestatusserver> getchatgroupdowninglist(std::string account);
    void transferinsert(std::string fileid,
                        std::string account,
                        std::string downpath,std::string ischat);
    void upfinish(std::string fileid);
    void downfinish(std::string fileid);
};