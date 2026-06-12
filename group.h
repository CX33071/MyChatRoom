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
#include <unordered_set>
#include <unordered_map>
#include <string>
class Group{
    private:
     cpp_redis::client redis_;
     std::unordered_map<std::string, std::unordered_set<std::string>> group_map;

    public:
     std::string creategroup(std::string account, std::string name);
     int invite(std::string account,
                 std::string invaccount,
                 std :: string name);
     int delgroup(std::string groupname, std::string account);
     bool agreejoin(std::string account, std::string groupname);
     bool refusejoin(std::string account,
                     std::string target,
                     std::string groupname);
     bool groupchat(std::string groupname,
                    std::string account,
                    std::string msg);
};