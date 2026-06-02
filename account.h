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
// std::unordered_map<std::string, int> uid1;
// std::unordered_map<int, std::string> uid2;
std::string cinkey() ;
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) ;
class verifycode {
   private:
    cpp_redis::client redis_;

   public:
    verifycode() ;
    std::string generatesalt() ;
    std::string sha(std::string input) ;
    std::string screctkey(std::string key) ;
    std::string getstartkey(std::string hashkey) ;
    bool checkkey(const std::string& inputkey, const std::string& getkeyvalue) ;
    std::string code() ;
    void addredis(const std::string& server, const std::string& account) ;
    void signup() ;
    bool verify(const std::string& account, const std::string& inputcode) ;
    void loginwithkey(const std::string& server, int fd,std::string account) ;
    void loginwithcode(const std::string& server, int fd,std::string account) ;
    void forgetkey(const std::string& server) ;
    void destroy(const std::string& server);
    void sendcom(const std::string& serveraccount,
                 const std::string clientaccount,
                 const std::string& subject,
                 const std::string code) ;
};
