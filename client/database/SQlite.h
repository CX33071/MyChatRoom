#pragma once
#include <sqlite3.h>
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
#include <mutex>
#include <string>
#include <vector>
class SQlite{
    public:
     SQlite();
     ~SQlite();
     void open(std::string name);
     void createtable();
     void addfriendchat(std::string sender,std::string reciver,std::string text);
     void addgroupchat(std::string groupname,std::string sender,std::string text);
     std::vector<std::string> getfriendchat(std::string account,std::string friendaccount);
     std::vector<std::string> getgroupchat(std::string groupname);
    private:
     sqlite3* db_;
};