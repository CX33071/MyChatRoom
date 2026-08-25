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
#include <mutex>
#include <string>
#include <vector>
class Mysql {
    private:
     MYSQL* mysql_;
     std::mutex mutex_;
    public:
     Mysql();
     ~Mysql();
     void exesql(const char* sql);
     std::vector<std::string> selectmul(const char* sql);
     bool select(const char* sql_);
     std::string selectstring(const char* sql);
     std::vector<std::string> selectrow(const char* sql_);
};
