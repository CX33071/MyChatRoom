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
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "json.hpp"
#include "Mysql.h"
#include "friend.h"
using json = nlohmann::json;
class FILEredis{
    private:
     cpp_redis::client redis_;
     Mysql mysql_;

    public:
     FILEredis();
     std::string begin(std::string from,
                std::string to,
                std::string filename,
                std::string filesize,std::string filepath);
     std::string finish(std::string ID);
     std::string getfrom(std::string ID);
     std::string getto(std::string ID);
     std::string getfilesize(std::string ID);
     std::string getuploaded(std::string fileid);
     void setloadfinish(std::string ID);
     std::vector<filestatusserver> getuploading(std::string sender);
     std::string getfilepath(std::string fileid);
};