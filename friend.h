#include "account.h"
bool sendn(int fd, char* data, ssize_t len) ;
class Friend {
   private:
    cpp_redis::client redis_;

   public:
    Friend();
    bool addapply(std::string applyaccount, std::string appliedaccount) ;
    bool agreeapply(std::string applyaccount, std::string appliedaccount) ;
    bool refuseapply(std::string applyaccount, std::string appliedaccount) ;
    bool block(std::string applyaccount, std::string appliedaccount) ;
    int cancleblock(std::string applyaccount, std::string appliedaccount) ;
    int delfriend(std::string applyaccount, std::string appliedaccount) ;
    bool isfriend(std::string applyaccount, std::string appliedaccount) ;
    bool isblock(std::string applyaccount, std::string appliedaccount) ;
    bool isonline(std::string account) ;
    std::vector<std::string> friendlist(std::string account) ;
    std::vector<std::string> onlienlist(std::string acount) ;
};
