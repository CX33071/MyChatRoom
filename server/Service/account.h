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
#include <mysql/mysql.h>
#include <string>
#include "group.h"
#include "friend.h"
#include "database/Mysql.h"
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) ;
class Verifycode {
   private:
    cpp_redis::client redis_;
    Mysql mysql_;

   public:
    Verifycode();
    std::string generatesalt();
    std::string sha(std::string input);
    std::string screctkey(std::string key);
    std::string getstartkey(std::string hashkey);
    bool checkkey(const std::string& inputkey, const std::string& getkeyvalue);
    std::string code();
    bool addredis(const std::string& account);
    bool signup(std::string account, std::string password,std::string name);
    int modifyname(std::string account, std::string name);
    bool verify(std::string account, std::string code);
    int loginwithkey(std::string account, std::string password);
    bool forgetkey(std::string account);
    int isexists(std::string account);
    bool is_online(std::string account);
    bool isexistsname(std::string name);
    void resetatstus();
    std::string getaccount(std::string name);
    std::string getname(std::string account);
    int destroy(std::string account,
                std::string password,
                Group& group,
                Friend& f);
    void exitlogin(std::string account);
    void sendcom(const std::string clientaccount,
                 const std::string& subject,
                 const std::string code);
};
