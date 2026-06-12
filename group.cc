#include "group.h"
 std::string Group::creategroup(std::string account, std::string name){
     auto fut = redis_.exists({name});
     redis_.sync_commit();
     int num = 1;
     int exists = fut.get().as_integer();
     while (exists) {
         name += std::to_string(num);
         num++;
         auto fut1 = redis_.exists({name});
         exists = fut1.get().as_integer();
     }
     redis_.set(name+"owner",account);
     redis_.sadd(name + "members", {account});
     redis_.sync_commit();
     group_map[name].insert(account);
     return name;
 }
int Group::invite(std::string account,
                 std::string invaccount,
                 std :: string name){
    auto fut = redis_.exists({name + "owner"});
    redis_.sync_commit();
    int exist = fut.get().as_integer();
    if(!exist){
        return 1;
    }
    auto fut1 = redis_.sismember(name + "members", invaccount);
    redis_.sync_commit();
    if(fut1.get().as_integer()){
        return 2;
    }
    redis_.set(name + "apply" + invaccount,account);
    redis_.sync_commit();
    return 0;
}
int Group::delgroup(std::string groupname, std::string account){
    std::string owner = groupname + "owner";
    std::string member = groupname + "members";
    auto fut1 = redis_.exists({owner});
    redis_.sync_commit();
    if(!fut1.get().as_integer()){
        return 1;
    }
    auto fut2 = redis_.get(owner);
    std::string s = fut2.get().as_string();
    if(s!=account){
        return 2;
    }
    redis_.del({owner, member});
    group_map.erase(groupname);
    return 0;
}
bool Group::agreejoin(std::string account, std::string groupname){
    std::string member = groupname + "members";
    std::string apply = groupname + "apply" + account;
    redis_.sadd(member, {account});
    redis_.del({apply});
    redis_.sync_commit();
    group_map[groupname].insert(account);
    return true;
}
bool Group::refusejoin(std::string account,
                     std::string target,
                     std::string groupname){
    std::string apply = groupname + "apply" + account;
    redis_.del({apply});
    redis_.sync_commit();
    return true;
}
bool Group::groupchat(std::string groupname,
                    std::string account,
                    std::string msg){
    redis_.rpush(groupname + "msg", {account + ":" + msg});
    return true;
}