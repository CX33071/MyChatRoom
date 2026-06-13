#include "friend.h"
bool sendn(int fd, char* data, ssize_t len) {
    while (len > 0) {
        ssize_t n = send(fd, data, len, 0);
        if (n <= 0) {
            return false;
        }
        data += n;
        len -= static_cast<ssize_t>(n);
    }
    return true;
}
     Friend::Friend() { 
        redis_.connect("127.0.0.1", 6379);
        redis_.sync_commit();
     }
    bool Friend::addapply(std::string applyaccount,std::string appliedaccount){
        auto fut1 = redis_.exists({appliedaccount});
        redis_.sync_commit();

        int num1 = fut1.get().as_integer();
        if(num1){
            redis_.sadd("apply" + applyaccount, {applyaccount});
            redis_.sadd("applied" + appliedaccount, {appliedaccount});
            redis_.sync_commit();
        } else {
            // std::cout << "该用户不存在" << std::endl;
            return false;
        }
        return true;
    }
    bool Friend::agreeapply(std::string applyaccount,std::string appliedaccount){
        redis_.sadd("friend" + applyaccount, {appliedaccount});
        redis_.sadd("friend" + appliedaccount, {applyaccount});
        redis_.srem("applied" + appliedaccount, {applyaccount});
        redis_.srem("apply" + applyaccount, {appliedaccount});
        redis_.sync_commit();
        return true;
    }
    bool Friend::refuseapply(std::string applyaccount, std::string appliedaccount){
        redis_.srem("applied" + appliedaccount, {applyaccount});
        redis_.srem("apply" + applyaccount, {appliedaccount});
        redis_.sync_commit();
        return true;
        }
    bool Friend::block(std::string applyaccount, std::string appliedaccount){
        redis_.sadd("block" + applyaccount, {appliedaccount});
        redis_.sadd("blocked" + appliedaccount, {applyaccount});
        redis_.sync_commit();
        return true;
    }
    int Friend::cancleblock(std::string applyaccount, std::string appliedaccount){
        auto fut = redis_.exists({appliedaccount});
        redis_.sync_commit();
        int num = fut.get().as_integer();
        if(!num){
            return 1;
        }
        auto fut1 = redis_.exists({"block" + applyaccount});
        int num1 = fut1.get().as_integer();
        if(!num1){
            return 2;
        }
        redis_.srem("block" + applyaccount, {appliedaccount});
        redis_.srem("blocked" + appliedaccount, {applyaccount});
        redis_.sync_commit();
        return 0;
    }
    int Friend::delfriend(std::string applyaccount, std::string appliedaccount){
        auto fut = redis_.exists({appliedaccount});
        int num = fut.get().as_integer();
        if(!num){
            return 1;
        }
        auto fut1 = redis_.exists({"friend" + appliedaccount});
        int num1 = fut1.get().as_integer();
        if(!num1){
            return 2;
        }
        redis_.srem("friend" + applyaccount, {appliedaccount});
        redis_.srem("friend" + appliedaccount, {applyaccount});
        redis_.sync_commit();
        std::cout << "好友删除成功!" << std::endl;
        return 0;
    }
    bool Friend::isfriend(std::string applyaccount, std::string appliedaccount){
        auto fut = redis_.exists({"friend" + applyaccount});
        redis_.sync_commit();

        int num = fut.get().as_integer();
        return num == 1;
    }
    bool Friend::isblock(std::string applyaccount, std::string appliedaccount){
        auto fut = redis_.exists({"block" + applyaccount});
        redis_.sync_commit();

        int num = fut.get().as_integer();
        return num == 1;
    }
    bool Friend::isonline(std::string account){
        auto fut = redis_.exists({"online" + account});
        redis_.sync_commit();

        int num = fut.get().as_integer();
        return num == 1;
    }
 
    std::vector<std::string> Friend::friendlist(std::string account){
        std::vector<std::string> list;
        auto futs = redis_.smembers("friend" + account);
        redis_.sync_commit();
        auto reply = futs.get();
        std::cout << "您的好友列表:" << std::endl;
        for (auto fut : reply.as_array()) {
            list.push_back(fut.as_string());
        }
        std::cout << "\n";
        return list;
    }
    std::vector<std::string> Friend::onlienlist(std::string account){
        std::vector<std::string> list;
        auto futs = redis_.smembers("online" + account);
        redis_.sync_commit();

        for(auto fut:futs.get().as_array()){
            list.push_back(fut.as_string());
        }
        return list;
    }
