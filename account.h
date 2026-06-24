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
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) ;
class Verifycode {
   private:
    cpp_redis::client redis_;

   public:
    Verifycode() ;
    std::string code() ;
    void addredis(const std::string& account) ;
    bool signup(std::string account,std::string password) ;
    bool verify(std::string account,std::string code) ;
    int loginwithkey(std::string account, std::string password);
    bool forgetkey(std::string account) ;
    int destroy(std::string account,std::string password);
    void sendcom(
                 const std::string clientaccount,
                 const std::string& subject,
                 const std::string code) ;
};
