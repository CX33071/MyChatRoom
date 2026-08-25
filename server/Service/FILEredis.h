#pragma once
#include <curl/curl.h>
#include <termios.h>
#include <unistd.h>
#include <algorithm>
#include <cpp_redis/cpp_redis>
#include <cstdlib>
#include "muduo/base/logger.h"
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
    std::string friendupbegin(std::string sender,std::string reciver,std::string filename,std::string filesize,std::string ischat,
std::string uppath);
    std::string groupupbegin(std::string sender,std::string groupname,std::string filename,std::string filesize,std::string ischat,
std::string uppath);
    std::string getsender(std::string fileid);
    std::string getreciver(std::string fileid);
    std::string getgroupname(std::string fileid);
    std::string getfilesize(std::string fileid);
    std::string getdownpath(std::string fileid,std::string account);
    std::string getuppath(std::string fileid);
    std::string getupsize(std::string fileid);
    std::string getdownsize(std::string fileid,std::string account);
    std::string getfilename(std::string fileid);
    void transferinsert(std::string fileid,
                        std::string account,
                        std::string downpath,std::string ischat,std::string newfilename);
    void upfinish(std::string fileid);
    void downfinish(std::string fileid);
    std::vector<json> getdownfilelist(std::string account,std::string ischat);
    std::vector<json> getgroupdownfilelist(std::vector<std::string> grouplist);
    std::vector<json> getchatgroupdownfilelist(
        std::vector<std::string> grouplist);
    std::string getnewfilename(std::string fileid,std::string account,std::string ischat);
    void deldownrecords(std::string ID,std::string account,std::string filepath,std::string ischat,std::string filename);
    std::vector<filestatusserver> getlist(std::string account,
std::string ischat,std::string status, bool group);
};