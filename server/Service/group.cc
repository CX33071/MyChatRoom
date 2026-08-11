#include "group.h"
Group::Group() {
    redis_.connect("127.0.0.1", 6379);
    redis_.sync_commit();
    std::string sql =
        "create table if not exists groupmember_info (id int auto_increment primary "
        "key,account varchar(30),groupname varchar(20));";
    mysql_.createinfo(sql.c_str());
    std::string sql1 =
        "create table if not exists owner_info (id int auto_increment primary "
        "key,groupname "
        "varchar(30),owner varchar(30));";
    mysql_.createinfo(sql1.c_str());
    std::string sql2 =
        "create table if not exists groupmanage_info (id int auto_increment "
        "primary key,groupname "
        "varchar(20),manager varchar(30));";
    mysql_.createinfo(sql2.c_str());
    std::string sql3 =
        "create table if not exists disconnectmsg_info (id int auto_increment "
        "primary "
        "key,account varchar(30),msg longtext);";
    mysql_.createinfo(sql3.c_str());
    std::string sql4 =
        "create table if not exists apply_info (id int auto_increment primary "
        "key,account "
        "varchar(20),groupname varchar(30));";
    mysql_.createinfo(sql4.c_str());
    std::string sql5 =
        "create table if not exists groupchat_history (id int auto_increment "
        "primary "
        "key,sender varchar(30),groupname varchar(30),context longtext,send_time "
        "datetime default current_timestamp);";
    mysql_.createinfo(sql5.c_str());
}
std::string Group::getname(std::string account) {
    auto fut = redis_.exists({account});
    redis_.sync_commit();
    if (!fut.get().as_integer()) {
        std::string sql =
            "select name from name_info where account ='" + account + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        if (!res.empty()) {
            redis_.set(account, res);
            redis_.sync_commit();
            return res;
        }
    }
    auto fut1 = redis_.get(account);
    redis_.sync_commit();
    std::string name = fut1.get().as_string();
    return name;
}
 std::string Group::creategroup(std::string account, std::string name){

     auto fut = redis_.exists({name+"owner:"});
     redis_.sync_commit();
     int num = 1;
     int exists = fut.get().as_integer();
     while (1) {
        if(exists){
            name += std::to_string(num);
            num++;
            auto fut1 = redis_.exists({name + "owner:"});
            redis_.sync_commit();

            exists = fut1.get().as_integer();
        }else{
            std::string sql =
                "select * from owner_info where groupname ='" + name + "'";
            std::string res = mysql_.selectstring(sql.c_str());
            if(res.empty()){
                break;
            }else{
                redis_.set(name + "owner", res);
                name += std::to_string(num);
                num++;
                auto fut2 = redis_.exists({name + "owner"});
                redis_.sync_commit();
                exists = fut2.get().as_integer();
            }
        }
     }
     redis_.set(name+"owner:",account);
     redis_.sadd("grouplist" + account, {name});
     redis_.sadd("ownergrouplist" + account, {name});
     redis_.sadd(name + "members", {account});
     redis_.sync_commit();
     std::string sql1 = "insert into owner_info (groupname,owner)values('" +
                        name + "','" + account + "')";
     mysql_.addmsg(sql1.c_str());
     std::string sql2 =
         "insert into groupmember_info (account,groupname)values('" + account +
         "','" + name + "')";
     mysql_.addmsg(sql2.c_str());
     group_map[name].insert(account);
     return name;
 }
 int Group::is_groupmember(std::string groupname, std::string account){
     auto f = redis_.sismember(groupname+"members",{account});
     redis_.sync_commit();
     int exists = f.get().as_integer();
     if(!exists){
         std::string sql = "select * from groupmember_info where account='" +
                           account + "'and groupname='" + groupname + "'";
         bool b = mysql_.select(sql.c_str());
         if(!b){
             exists = 0;
         }else{
             exists = 1;
         }
     }
     return exists;
 }
 int Group::is_existsgroup(std::string groupname){
     auto f = redis_.exists({groupname + "owner:"});
     redis_.sync_commit();
     int exists = f.get().as_integer();
     if(!exists){
         std::string sql =
             "select *from owner_info where groupname ='" + groupname + "'";
         bool b = mysql_.select(sql.c_str());
        if(!b){
            exists = 0;
        }else{
            exists = 1;
        }
     }
     return exists;
 }
 int Group::is_manager(std::string groupname, std::string account) {
     auto f = redis_.sismember(groupname+"managers",{account});
     redis_.sync_commit();
     auto f1 = redis_.get({groupname + "owner:"});
     redis_.sync_commit();
     std::string owner = f1.get().as_string();
     if(owner.empty()){
         std::string sql1 = "select owner from owner_info where groupname ='" +
                            groupname + "'";
        std::string res= mysql_.selectstring(sql1.c_str());
        owner = res;
     }
     redis_.sync_commit();
     int exists = f.get().as_integer();
     if(!exists){
         std::string sql = "select *from groupmanage_info where groupname='" +
                           groupname + "'and manager ='" + account + "'";
        bool b= mysql_.select(sql.c_str());
         if(!b){
             exists = 0;
         }else{
             exists = 1;
         }
     }
     return exists||owner==account;
 }
 void Group::addmanager(std::string groupname, std::string account) {
     redis_.sadd(groupname + "managers", {account});
     redis_.sync_commit();
     std::string sql = "insert into groupmanage_info (groupname,manager)values ('" +
                       groupname + "','" + account + "')";
     mysql_.addmsg(sql.c_str());
 }
 void Group::delmanager(std::string groupname, std::string account) {
     redis_.srem(groupname + "managers",{account});
     redis_.sync_commit();
     std::string sql = "delete from groupmanage_info where groupname = '" +
                       groupname + "'and manager ='" + account + "'";
     mysql_.delmsg(sql.c_str());
 }
 int Group::exitgroup(std::string account, std::string groupname){
     auto f1 = redis_.exists({groupname + "owner:"});
     std::string owner;
     redis_.sync_commit();
     int exists = f1.get().as_integer();
     if(!exists){
        std::string sql="select owner from owner_info where groupname ='"+groupname+"'";
         owner=mysql_.selectstring(sql.c_str());
        if(owner.empty()){
            return 2;
        }
     }else{
         f1 = redis_.get(groupname + "owner:");
         redis_.sync_commit();
         owner = f1.get().as_string();
     }
     if(owner==account){
         delgroup(groupname, account);
         return 0;
     }
     auto f2 = redis_.sismember("grouplist" + account, {groupname});
     redis_.sync_commit();
     exists = f2.get().as_integer();
     if(!exists){
         std::string sql1 =
             "select * from groupmember_info where groupname ='" + groupname +
             "' and account = '" + account + "'";
         bool b = mysql_.select(sql1.c_str());
        if(!b){
            return 1;
        }
     }
     redis_.srem("grouplist" + account, {groupname});
     redis_.srem(groupname + "members", {account});
     redis_.srem(groupname + "managers", {account});
     redis_.sync_commit();
     std::string sql3 = "delete from groupmember_info where groupname = '" +
                        groupname + "'and account = '" + account + "'";
     mysql_.delmsg(sql3.c_str());
     return 0;
 }
std::vector<std::string> Group::
     groupmembers(std::string account, std::string groupname) {
     std::vector<std::string> members;
     auto f3 = redis_.exists({groupname + "owner:"});
     redis_.sync_commit();
     int exists = f3.get().as_integer();
     if(!exists){
         std::string sql =
             "select owner from owner_info where groupname ='" +
             groupname + "'";
         bool b=mysql_.select(sql.c_str());
         if(!b){
             members.push_back("NULL");
             return members;
         }
     }
     auto f = redis_.sismember("grouplist" + account, groupname);
     redis_.sync_commit();
     if(!f.get().as_integer()){
         std::string sql1 =
             "select *from groupmember_info where groupname = '" + groupname +
             "'and account ='" + account + "'";
         bool b = mysql_.select(sql1.c_str());
         if(!b){
             return members;
         }
     }
     std::string sql2, sql3;
     auto s = redis_.exists({groupname + "managers"});
     redis_.sync_commit();
     if (!s.get().as_integer()) {
         sql2 = "select manager from groupmanage_info where groupname ='" +
                groupname + "'";
         std::vector<std::string> mem = mysql_.selectmul(sql2.c_str());
         redis_.sadd(groupname + "managers", {mem});
         redis_.sync_commit();
     }
     auto f4 = redis_.smembers(groupname + "managers");
     redis_.sync_commit();
     auto reply1 = f4.get();
     for (auto it : reply1.as_array()) {
         std::string name = getname(it.as_string());
         members.push_back("管理员:" + name);
     }
     auto s1 = redis_.exists({groupname + "members"});
     redis_.sync_commit();
     if (!s1.get().as_integer()) {
         sql3 = "select account from groupmember_info where groupname ='" +
                groupname + "'";
         std::vector<std::string> mems = mysql_.selectmul(sql3.c_str());
         redis_.sadd(groupname + "members", {mems});
         redis_.sync_commit();
         return mems;
     }
     auto f2 = redis_.smembers(groupname + "members");
     redis_.sync_commit();
     auto reply = f2.get();
     std::string name;
     for (auto it : reply.as_array()) {
         name = getname(it.as_string());
         members.push_back(name);
     }
     return members;
 }
 std::vector<std::string> Group::grouptargetmember(std::string groupname,std::string account){
     auto s = redis_.exists({groupname + "members"});
     redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql = "select account from groupmember_info where groupname ='" +
               groupname + "'";
        std::vector<std::string> mems = mysql_.selectmul(sql.c_str());
        redis_.sadd(groupname + "members", {mems});
        redis_.sync_commit();
    }
     auto f = redis_.smembers(groupname + "members");
     redis_.sync_commit();
     std::vector<std::string> res;
     auto reply = f.get();
     for (auto it : reply.as_array()) {
         std::string s;
         s = it.as_string();
         if (s != account) {
             res.push_back(it.as_string());
         }
    }
    return res;
 }
 std::vector<std::string> Group::grouplist(std::string account){
     std::vector<std::string> list;
     auto s = redis_.exists({"grouplist" + account});
     redis_.sync_commit();
     if(!s.get().as_integer()){
         std::string sql =
             "select groupname from groupmember_info where account ='" +
             account + "'";
         std::vector<std::string> mem = mysql_.selectmul(sql.c_str());
         redis_.sadd("grouplist" + account, {mem});
         redis_.sync_commit();
         return mem;
     }
     auto futs = redis_.smembers("grouplist" + account);
     redis_.sync_commit();
     auto reply = futs.get();
     for (auto fut : reply.as_array()) {
         list.push_back(fut.as_string());
     }
     return list;
 }
std::vector<std::string> Group::ownergrouplist(std::string account){
    std::vector<std::string> list;
    auto futs = redis_.smembers("ownergrouplist" + account);
    redis_.sync_commit();
    auto reply = futs.get();
    for (auto fut : reply.as_array()) {
        list.push_back(fut.as_string());
    }
    return list;
}
int Group::delgroup(std::string groupname, std::string account){
    std::string owner = groupname + "owner:";
    auto fut1 = redis_.exists({owner});
    redis_.sync_commit();
    std::string res;
    if (!fut1.get().as_integer()) {
        std::string sql =
            "select owner from owner_info where groupname ='" + groupname + "'";
         res  = mysql_.selectstring(sql.c_str());
        if(res.empty()){
            return 1;
        }else{
            redis_.set(groupname + "owner:", res);
            redis_.sync_commit();
        }
    }
    auto fut2 = redis_.get(owner);
    redis_.sync_commit();
    res = fut2.get().as_string();
    if(res!=account){
        return 2;
    }
    std::string member = groupname +  "members";
    auto fut3 = redis_.smembers(member);
    redis_.sync_commit();
    std::vector<std::string> m;
    auto r = fut3.get();
    for (auto f : r.as_array()) {
        m.push_back(f.as_string());
    }
    for(auto m1:m){
        redis_.srem("grouplist"+m1,{groupname});
        redis_.sync_commit();
    }
    redis_.del({owner, member});
    redis_.del({"groupchat" + groupname});
    redis_.del({"groupchatsender" + groupname});
    redis_.srem("ownergrouplist" + account, {groupname});
    redis_.sync_commit();
    std::string sql2;
    sql2 = "delete from groupmember_info where groupname ='" + groupname + "'";
    std::string sql3 =
        "delete from owner_info where groupname ='" + groupname + "'";
    std::string sql4 =
        "delete from groupmanage_info where groupname ='" + groupname + "'";
    std::string sql5 =
        "delete from groupchat_history where groupname ='" + groupname + "'";
    mysql_.delmsg(sql3.c_str());
    mysql_.delmsg(sql4.c_str());
    mysql_.delmsg(sql5.c_str());
    mysql_.delmsg(sql2.c_str());
    group_map.erase(groupname);
    return 0;
}
 bool Group::agreejoin(
    std::string account,
    std::string groupname) {
     redis_.srem("appliedjoingroup" + groupname, {account});
     redis_.sadd("grouplist" + account, {groupname});
     redis_.sadd(groupname+"members",{account});
     redis_.sync_commit();
     std::string sql1 = "delete from apply_info where groupname ='" +
                        groupname + "'and account ='" + account + "'";
     mysql_.delmsg(sql1.c_str());
     std::string sql2 =
         "insert into groupmember_info (groupname,account)values('" +
         groupname + "','" + account + "')";
     mysql_.addmsg(sql2.c_str());
     group_map[groupname].insert(account);
     return true;
}
bool Group::refusejoin(std::string account,
                     std::string groupname){
    redis_.srem("appliedjoingroup" + groupname, {account});
    redis_.sync_commit();
    std::string sql = "delete from apply_info where groupname ='" +
                      groupname + "'and account = '" + account + "'";
    mysql_.delmsg(sql.c_str());
    return true;
}
std::vector<std::string> Group::applyjoingroup(std::string account,std::string groupname){
    std::vector<std::string> res;
    auto s=redis_.exists({groupname+"managers"});
    redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql1 =
            "select manager from groupmanage_info where groupname ='" +
            groupname + "'";
        std::vector<std::string> mems = mysql_.selectmul(sql1.c_str());
        redis_.sadd(groupname+"managers",{mems});
        redis_.sync_commit();
    }
    auto f = redis_.smembers(groupname + "managers");
    auto s2 = redis_.exists({groupname + "owner:"});
    redis_.sync_commit();
    if(!s2.get().as_integer()){
        std::string sql2 =
            "select owner from owner_info where groupname ='" + groupname + "'";
        std::string owner = mysql_.selectstring(sql2.c_str());
        redis_.set(groupname + "owner:", owner);
        redis_.sync_commit();
    }
    auto f1 = redis_.get(groupname + "owner:");
    redis_.sadd("appliedjoingroup" + groupname, {account});
    redis_.sync_commit();
    std::string sql3 = "insert into apply_info (groupname,account)values('" +
                       groupname + "','" + account + "')";
    mysql_.addmsg(sql3.c_str());
    auto reply1 = f.get();
    for (auto it : reply1.as_array()) {
        res.push_back(it.as_string());
    }
    res.push_back(f1.get().as_string());
    return res;
}
std::string Group::findowner(std::string groupname){
    auto s=redis_.exists({groupname+"owner:"});
    redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql =
            "select owner from owner_info where groupname ='" + groupname + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        redis_.set(groupname + "owner:", res);
        redis_.sync_commit();
    }
    auto f = redis_.get(groupname + "owner:");
    redis_.sync_commit();
    std::string owner = f.get().as_string();
    return owner;
}
void Group::delmember(std::string groupname,std::string account){
    redis_.srem("grouplist" + account, {groupname});
    redis_.sync_commit();
    redis_.srem(groupname + "members", {account});
    redis_.sync_commit();
    std::string sql1 = "delete from groupmember_info where groupname = '" +
                       groupname + "'and account ='" + account + "'";
    mysql_.delmsg(sql1.c_str());
    auto s = redis_.exists({groupname + "managers"});
    redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql2 =
            "select manager from groupmanage_info where groupname = '" + groupname +
            "'";
        std::vector<std::string> mems=mysql_.selectmul(sql2.c_str());
        redis_.sadd(groupname + "managers", {mems});
        redis_.sync_commit();
    }
    auto f = redis_.sismember(groupname + "managers", {account});
    redis_.sync_commit();

    if(f.get().as_integer()){
        redis_.srem(groupname + "managers", {account});
        redis_.sync_commit();
    }
    std::string sql7 = "delete from groupmanage_info where groupname ='" +
                       groupname + "'and manager ='" + account + "'";
    mysql_.delmsg(sql7.c_str());
}
void Group::historygroupchat(std::string account,
                      std::string groupname,std::string name,
                      std::string content) {
    json j;
    j["sender"] = name;
    j["content"] = content;
    std::string s = j.dump();
    redis_.rpush("groupchat" + groupname, {s});
    redis_.sync_commit();
    std::string sql =
        "insert into groupchat_history (sender,groupname,context)values('" +
        account + "','" + groupname + "','" + content+ "')";
    mysql_.addmsg(sql.c_str());
}
std::vector<friendchatrecord> Group::getgrouphistory(std::string groupname){
    std::vector<friendchatrecord> res;
    auto s = redis_.exists({"groupchat" + groupname});
    redis_.sync_commit();
    if (!s.get().as_integer()) {
        std::string sql1 = "select sender from groupchat_history where groupname ='"+groupname+"'order by send_time";
        std::vector<std::string> his = mysql_.selectmul(sql1.c_str());
        std::string sql2 =
            "select content from groupchat_history where groupname ='" +
            groupname + "' order by send_time";
        std::vector<std::string> his1=mysql_.selectmul(sql2.c_str());
        for (int i = 0; i < his.size();i++){
            json j1;
            j1["sender"] = his[i];
            j1["content"] = his1[i];
            friendchatrecord fr;
            fr.sender = his[i];
            fr.content=his1[i];
            std::string s = j1.dump();
            redis_.rpush("groupchat" + groupname, {s});
            redis_.sync_commit();
            res.push_back(fr);
        }
        redis_.sync_commit();
        return res;
    }
    auto f = redis_.lrange("groupchat" + groupname, 0, -1);
    redis_.sync_commit();
    auto reply = f.get();
    for(auto it:reply.as_array()){
        json j = json::parse(it.as_string());
        friendchatrecord fr;
        fr.sender=j["sender"];
        fr.content = j["content"];
        res.push_back(fr);
    }
    return res;
}
void Group::disconnectmsg(std::string account,json j){
    redis_.sadd("disconnectmsg" + account, {j.dump()});
    redis_.sync_commit();
    std::string sql = "insert into disconnectmsg_info (account,msg)values('" +
                      account + "','" + j.dump() + "')";
    mysql_.addmsg(sql.c_str());
}
std::vector<std::string> Group::getdisconnectmsg(std::string account){
    auto s = redis_.exists({"disconnectmsg" + account});
    redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql =
            "select msg from disconnectmsg_info where account ='" + account +
            "'";
        std::vector<std::string> msgs = mysql_.selectmul(sql.c_str());
        redis_.sadd("disconnectmsg" + account, {msgs});
        redis_.sync_commit();
        return msgs;
    }
    auto f = redis_.smembers("disconnectmsg" + account);
    redis_.sync_commit();
    std::vector<std::string> res;
    auto reply = f.get();
    for (auto it : reply.as_array()) {
        res.push_back(it.as_string());
    }
    return res;
}
void Group::destorydismsg(std::string account){
    redis_.del({"disconnectmsg" + account});
    redis_.sync_commit();
    std::string sql =
        "delete from disconnectmsg_info where account ='" + account + "'";
    mysql_.delmsg(sql.c_str());
}