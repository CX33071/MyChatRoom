#pragma once
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
    std::string sql =
        "create table if not exists friend_info(id int auto_increment primary key,account "
        "varchar(30) ,friendaccount varchar(30));";
    mysql_.createinfo(sql.c_str());
    sql =
        "create table if not exists applyaddfriend_info(id int auto_increment primary "
        "key,appaccount varchar(30),appedaccount varchar(30));";
    mysql_.createinfo(sql.c_str());
    sql =
        "create table if not exists block_info(id int auto_increment primary key,account "
        "varchar(30),blockfriend varchar(30));";
    mysql_.createinfo(sql.c_str());
    sql =
        "create table if not exists friendchat_history(id int auto_increment primary key,sender varchar(30),reciver varchar(30),content text,"
        "send_time datetime default current_timestamp);";
    // mysql_.createinfo(sql.c_str());
    // sql =
    //     "create table if not exists friendname_info (id int increment primary "
    //     "key,name varchar(100),friendname varchar(100));";
    // mysql_.createinfo(sql.c_str());
}
std::string Friend::getname(std::string account){
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
bool Friend::addapply(std::string applyaccount, std::string appliedaccount) {
    auto fut1 = redis_.exists({appliedaccount + "key"});
    redis_.sync_commit();

    int num1 = fut1.get().as_integer();
    if(!num1){
        std::string sql =
            "select password from account_info where account='" + appliedaccount + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        if (res.empty()) {
            return false;
        } else {
            redis_.set(appliedaccount + "key", res);
            redis_.sync_commit();
        }
    }
        redis_.sadd("addfriend" + applyaccount, {appliedaccount});
        redis_.sadd("addedfriend" + appliedaccount, {applyaccount});
        redis_.sync_commit();
        std::string sql =
            "insert into applyaddfriend_info "
            "(appaccount,appedaccount)values('" +
            applyaccount + "','" + appliedaccount + "')";
        mysql_.addmsg(sql.c_str());
        return true;
}
bool Friend::agreeapply(std::string applyaccount, std::string appliedaccount) {
    redis_.sadd("friend" + applyaccount, {appliedaccount});
    redis_.sadd("friend" + appliedaccount, {applyaccount});
    redis_.srem("addedfriend" + appliedaccount, {applyaccount});
    redis_.srem("addfriend" + applyaccount, {appliedaccount});
    redis_.sync_commit();
    std::string sql1, sql2,sql3;
    sql1 = "insert into friend_info (account,friendaccount)values('" +
           applyaccount + "','" + appliedaccount + "')";
    sql3 = "insert into friend_info (account,friendaccount)values('" +
           appliedaccount + "','" + applyaccount + "')";
    sql2 = "delete from applyaddfriend_info where appaccount = '" +
           applyaccount + "' and appedaccount = '" + appliedaccount + "'";
    mysql_.addmsg(sql1.c_str());
    mysql_.delmsg(sql2.c_str());
    mysql_.addmsg(sql3.c_str());
    return true;
}
bool Friend::refuseapply(std::string applyaccount, std::string appliedaccount) {
    redis_.srem("addedfriend" + appliedaccount, {applyaccount});
    redis_.srem("addfriend" + applyaccount, {appliedaccount});
    redis_.sync_commit();
    std::string sql="delete from applyaddfriend_info where appaccount = '" + applyaccount +
        "' and appedaccount = '" + appliedaccount + "'";
    mysql_.delmsg(sql.c_str());
    return true;
}
bool Friend::block(std::string applyaccount, std::string appliedaccount) {
    redis_.sadd("block" + applyaccount, {appliedaccount});
    redis_.sadd("blocked" + appliedaccount, {applyaccount});
    redis_.sync_commit();
    std::string sql = "insert into block_info (account,blockfriend) values('" +
                      applyaccount + "','" + appliedaccount + "')";
    mysql_.addmsg(sql.c_str());
    return true;
}
int Friend::cancleblock(std::string applyaccount, std::string appliedaccount) {
    redis_.srem("block" + applyaccount, {appliedaccount});
    redis_.srem("blocked" + appliedaccount, {applyaccount});
    redis_.sync_commit();
    std::string sql = "delete from block_info where account='" + applyaccount +
                      "'and blockfriend ='" + appliedaccount + "'";
    mysql_.delmsg(sql.c_str());
    return 0;
}
int Friend::delfriend(std::string applyaccount, std::string appliedaccount) {
    auto fut = redis_.exists({appliedaccount + "key"});
    redis_.sync_commit();
    int num = fut.get().as_integer();
    if (!num) {
        std::string sql = "select password from account_info where account='" +
                          appliedaccount + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        if (res.empty()) {
            return 1;
        } else {
            redis_.set(appliedaccount + "key", res);
            redis_.sync_commit();
        }
    }
    
    auto fut1 = redis_.sismember("friend" + applyaccount, appliedaccount);
    redis_.sync_commit();
    int num1 = fut1.get().as_integer();
    if (!num1) {
        std::string sql1 = "select * from friend_info where account ='" +
                           applyaccount + "'and friendaccount='" + appliedaccount +
                           "'";
        std::string result = mysql_.selectstring(sql1.c_str());
        if(result.empty()){
            return 2;
        }else{
            redis_.sadd("friend" + applyaccount, {appliedaccount});
            redis_.sadd("friend" + appliedaccount, {applyaccount});
            redis_.sync_commit();
        }
    }

    redis_.srem("friend" + applyaccount, {appliedaccount});
    redis_.srem("friend" + appliedaccount, {applyaccount});
    redis_.del({"friendchat" + applyaccount + appliedaccount,"friendchat"+appliedaccount+applyaccount});
    redis_.sync_commit();
    std::string sql2 = "delete from friend_info where account='" +
                       applyaccount + "'and friendaccount = '" +
                       appliedaccount + "'";
    std::string sql3 = "delete from friend_info where account ='" +
                       appliedaccount + "'and friendaccount='" + applyaccount +
                       "'";
    std::string sql4 = "delete from friendchat_history where sender ='" +
                       applyaccount + "'and reciver ='" + appliedaccount + "'";
    std::string sql5 = "delete from friendchat_history where sender ='" +
                       appliedaccount + "'and reciver ='" + applyaccount + "'";
    mysql_.delmsg(sql2.c_str());
    mysql_.delmsg(sql3.c_str());
    mysql_.delmsg(sql4.c_str());
    mysql_.delmsg(sql5.c_str());
    return 0;
}
void Friend::delfriend1(std::string account, std::string target) {
    redis_.srem("friend" + account, {target});
    redis_.sync_commit();
    std::string sql = "delete from friend_info where account='" + account +
                      "'and friendaccount = '" + target + "'";
    mysql_.delmsg(sql.c_str());
}
void Friend::addfriend(std::string account, std::string target) {
    redis_.sadd("friend" + account, {target});
    redis_.sync_commit();
    std::string sql =
        "insert into friend_info (account,friendaccount)values('" + account +
        "','" + target + "')";
    mysql_.addmsg(sql.c_str());
}
int Friend::isfriend(std::string account, std::string friendaccount) {
    auto fut1 = redis_.exists({friendaccount + "key"});
    redis_.sync_commit();
    int exists = fut1.get().as_integer();
    if (!exists) {
        std::string sql = "select password from account_info where account='" +
                          friendaccount + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        if (res.empty()) {
            return 1;
        } else {
            redis_.set(friendaccount + "key", res);
            redis_.sync_commit();
        }
    }
    std::string sql1 = "select * from friend_info where account ='" + account +
                       "'and friendaccount='" + friendaccount + "'";
    bool b = mysql_.select(sql1.c_str());
    if (b) {
        redis_.sadd("friend" + account, {friendaccount});
        redis_.sync_commit();
    }
    auto fut = redis_.sismember("friend" + account, friendaccount);
    redis_.sync_commit();

    int num = fut.get().as_integer();
    if (num) {
        return 2;
    }
    return 0;
}
int Friend::isblock(std::string applyaccount, std::string appliedaccount) {
    auto fut1 = redis_.exists({appliedaccount + "key"});
    redis_.sync_commit();
    int exists = fut1.get().as_integer();
    if (!exists) {
        std::string sql = "select password from block_info where account='" +
                          appliedaccount + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        if (res.empty()) {
            return 1;
        } else {
            redis_.set(appliedaccount + "key", res);
            redis_.sync_commit();
        }
    }
    std::string sql1 = "select * from block_info where account ='" + applyaccount +
                       "'and blockfriend='" + appliedaccount + "'";
    bool b = mysql_.select(sql1.c_str());
    if (b) {
        redis_.sadd("block" + applyaccount, {appliedaccount});
        redis_.sync_commit();
    }
    auto fut = redis_.sismember("block" + applyaccount, appliedaccount);
    redis_.sync_commit();

    int num = fut.get().as_integer();
    if (num) {
        return 2;
    }
    return 0;
}
std::vector<std::string> Friend::friendlist(std::string account) {
    std::vector<std::string> list;
    auto fut = redis_.exists({"friend" + account});
    redis_.sync_commit();
    if (!fut.get().as_integer()) {
        std::string sql =
            "select friendaccount from friend_info where account='" + account +
            "'";
        std::vector<std::string> friends = mysql_.selectmul(sql.c_str());
        for (auto f : friends) {
            redis_.sadd("friend" + account, {f});
            redis_.sync_commit();
            list.push_back(f);
        }
        redis_.sync_commit();
        return list;
    }
    auto fut1 = redis_.smembers("friend" + account);
    redis_.sync_commit();
    auto reply = fut1.get();
    for (auto it : reply.as_array()) {
        list.push_back(it.as_string());
    }
    return list;
}
std::vector<std::string> Friend::blocklist(std::string account) {
    std::vector<std::string> list;
    auto fut = redis_.exists({"block" + account});
    redis_.sync_commit();
    if (!fut.get().as_integer()) {
        std::string sql = "select blockfriend from block_info where account='" +
                          account + "'";
        std::vector<std::string> blocks = mysql_.selectmul(sql.c_str());
        for (auto f : blocks) {
            redis_.sadd("block" + account, {f});
            redis_.sync_commit();
            list.push_back(f);
        }
        redis_.sync_commit();
        return list;
    }
    auto fut1 = redis_.smembers("block" + account);
    redis_.sync_commit();
    auto reply = fut1.get();
    for (auto it : reply.as_array()) {
        list.push_back(it.as_string());
    }
    return list;
}
std::vector<std::string> Friend::onlinelist(std::string account) {
    std::vector<std::string> list;
    auto fut1 = redis_.exists({"friend" + account});
    redis_.sync_commit();
    if (!fut1.get().as_integer()) {
        std::string sql =
            "select friendaccount from friend_info where account='" + account +
            "'";
        std::vector<std::string> friends = mysql_.selectmul(sql.c_str());
        for (auto f : friends) {
            redis_.sadd("friend" + account, {f});
            redis_.sync_commit();
        }
        redis_.sync_commit();
    }
    auto fut2 = redis_.smembers("friend" + account);
    redis_.sync_commit();
    auto reply = fut2.get();
    for (auto it : reply.as_array()) {
        std::string friend_account = it.as_string();
        auto fut3 = redis_.exists({"online" + friend_account});
        redis_.sync_commit();
        std::string status;
        if (!fut3.get().as_integer()) {
            std::string sql =
                "select online from account_online_info where account='" +
                friend_account + "'";
            status = mysql_.selectstring(sql.c_str());
            if (!status.empty()) {
                redis_.set("online" + friend_account, status);
                redis_.sync_commit();
            } else {
            }
        } else {
            auto fut4 = redis_.get("online" + friend_account);
            redis_.sync_commit();
            status = fut4.get().as_string();
        }
        if (status == "1"){
            std::string name = getname(friend_account);
            list.push_back(name + "     [在线]");
        }
        else{
            std::string name = getname(friend_account);
            list.push_back(name + "     [离线]");
        }
    }
    return list;
}
void Friend::historyfriendchat(std::string account1,
                               std::string account2,
                               std::string content) {
    redis_.rpush("friendchat" + account1 + account2, {content});
    redis_.rpush("friendchat" + account2 + account1, {content});
    redis_.sync_commit();
    std::string sql =
        "insert into friendchat_history(sender,reciver,content) values('" +
        account1 + "','" + account2 + "','" + content + "')";
    mysql_.addmsg(sql.c_str());
}
std::vector<std::string> Friend::gethistoryfriendchat(std::string account1,
                                                      std::string account2) {
    std::vector<std::string> historymsg;
    auto fut1 = redis_.exists({"friendchat" + account1 + account2});
    redis_.sync_commit();
    if (!fut1.get().as_integer()) {
        std::string sql =
            "select sender,content from friendchat_history "
            "where (sender='" +
            account1 + "' and reciver='" + account2 + "') or (sender='" +
            account2 + "' and reciver='" + account1 + "') order by send_time";
        std::vector<std::string> msgs = mysql_.selectmul2(sql.c_str());
        for (auto msg : msgs) {
            redis_.rpush("friendchat" + account1 + account2, {msg});
            historymsg.push_back(msg);
            redis_.sync_commit();
        }
        redis_.sync_commit();
        return historymsg;
    }
    auto fut2 = redis_.lrange("friendchat" + account1 + account2, 0, -1);
    redis_.sync_commit();
    auto reply = fut2.get();
    for (auto it : reply.as_array()) {
        historymsg.push_back(it.as_string());
    }
    return historymsg;
}
