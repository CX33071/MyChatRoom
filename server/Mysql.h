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
#include <string>
class Mysql {
    private:
     MYSQL* mysql_;
    public:
     Mysql();
     void createinfo(const char*sql_);
     void addmsg(const char* sql_);
     void delmsg(const char* sql_);
     void changemsg(const char* sql_);
     std::vector<std::string> selectmul(const char* sql);
     bool select(const char* sql_);
     std::string selectstring(const char* sql);
     std::vector<std::string>selectmul2(const char* sql_);
};