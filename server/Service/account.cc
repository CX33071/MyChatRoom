#include "account.h"
std::string server = "3541053286@qq.com";
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* data = (std::string*)userp;
    size_t len = data->size();
    memcpy(ptr, data->c_str(), len);
    data->clear();
    return len;
}
Verifycode::Verifycode() {
    srand(time(NULL));
    redis_.connect("127.0.0.1", 6379);
    redis_.sync_commit();
    std::string sql =
        "create table if not exists account_info(id int auto_increment primary "
        "key,account varchar(30) not null,password varchar(50));";
    mysql_.createinfo(sql.c_str());
    sql =
        "create table if not exists account_online_info(id int auto_increment "
        "primary key,account varchar(30) not null,online int); ";
    mysql_.createinfo(sql.c_str());
    sql =
        "create table if not exists name_info (id int auto_increment primary "
        "key,name varchar(100),account varchar(50));";
    mysql_.createinfo(sql.c_str());
    sql =
        "create table if not exists onlinename_info (id int auto_increment primary "
        "key,name varchar(100),online int);";
    mysql_.createinfo(sql.c_str());
}
void Verifycode::resetatstus(){
    std::string sql = "update account_online_info set online=0;";
    mysql_.changemsg(sql.c_str());
    sql = "update onlinename_info set online=0;";
    mysql_.changemsg(sql.c_str());
    std::vector<std::string> list;
   auto fut = redis_.keys("online*");
   redis_.sync_commit();
   auto reply1 = fut.get();
   for (auto it : reply1.as_array()) {
       list.push_back(it.as_string());
   }
    for (auto t : list) {
        redis_.del({t});
    }
    auto fut1=redis_.keys("onlinename*");
    redis_.sync_commit();
    auto reply = fut1.get();
    for (auto it : reply.as_array()) {
        list.push_back(it.as_string());
    }
    for (auto t : list) {
        redis_.del({t});
    }
    redis_.sync_commit();
}
bool Verifycode::isexistsname(std::string name){
    auto fut = redis_.exists({name});
    redis_.sync_commit();
    if (!fut.get().as_integer()) {
        std::string sql =
            "select account from name_info where name ='" + name + "'";
       std::string account= mysql_.selectstring(sql.c_str());
       if(!account.empty()){
           std::cout << "name =" << name << "对应的account =" << account
                     << std::endl;
           redis_.set(name, account);
           redis_.sync_commit();
           return true;
       }else{
           return false;
       }
    }
    return true;
}
std::string Verifycode::code() {
    std::string code;
    for (int i = 0; i < 4; i++) {
        code += ('0' + rand() % 10);
    }
    return code;
}
std::string Verifycode::getaccount(std::string name){
    auto fut = redis_.exists({name});
    redis_.sync_commit();
    if (!fut.get().as_integer()) {
        std::string sql="select account from name_info where name ='"+name+"'";
        std::string res = mysql_.selectstring(sql.c_str());
        if(!res.empty()){
            redis_.set(name, res);
            redis_.sync_commit();
            return res;
        }
    }
   auto fut1= redis_.get(name);
   redis_.sync_commit();
   std::string account = fut1.get().as_string();
   return account;
}
std::string Verifycode::getname(std::string account){
    auto fut=redis_.exists({account});
    redis_.sync_commit();
    if(!fut.get().as_integer()){
        std::string sql =
            "select name from name_info where account ='" + account + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        if(!res.empty()){
            redis_.set(account, res);
            redis_.sync_commit();
            return res;
        }
    }
    auto fut1=redis_.get(account);
    redis_.sync_commit();
    std::string name = fut1.get().as_string();
    return name;
}
int Verifycode::modifyname(std::string account,std::string name){
    auto fut=redis_.get(account);
    redis_.sync_commit();
    std::string oldname = fut.get().as_string();
    if(oldname==name){
        return 1;
    }
    redis_.set(name, account);
    redis_.set(account, name);
    redis_.set("onlinename" + name, "1");
    redis_.del({"onlinename"+oldname});
    redis_.sync_commit();
    std::string sql = "update onlinename_info set name ='" + name +
                      "' where name ='" + oldname + "'";
    mysql_.changemsg(sql.c_str());
    std::string sql1 = "update name_info set name ='" + name +
                       "'where account ='" + account + "'";
    mysql_.changemsg(sql1.c_str());
    return 0;
}
int Verifycode::isexists(std::string account){
    auto t = redis_.exists({account + "key"});
    redis_.sync_commit();
    if (!t.get().as_integer()) {
        std::string sql = "select password from account_info where account ='" +
                          account + "'";
        std::string password = mysql_.selectstring(sql.c_str());
        if(!password.empty()){
            redis_.set(account + "key", password);
            redis_.sync_commit();
            return 1;
        }else{
            return 2;
        }
    }
    return 1;
}
bool Verifycode::addredis(
                          const std::string& account) {
    auto fut = redis_.exists({account + "key"});
    redis_.sync_commit();
    int exists = fut.get().as_integer();
    if(!exists){
        std::string sql =
            "select password from account_info where account='" + account + "'";
            std::string res = mysql_.selectstring(sql.c_str());
        if(res.empty()){
            return false;
        }else{
            redis_.set(account + "key", res);
            redis_.sync_commit();
        }
    }
    std::string s = code();
    redis_.setex(account + "code", 300, s);
    redis_.sync_commit();
    sendcom( account, "验证码", "您的验证码是: " + s + " 5分钟内有效");
    return true;
}
bool Verifycode::signup(std::string account,std::string password,std::string name) {
    std::string sql = "select password from account_info where account='"+account+"'";
    std::string res = mysql_.selectstring(sql.c_str());
    if (!res.empty()) {
        redis_.set(account + "key", res);
        redis_.sync_commit();
    }
    auto fut = redis_.exists({account+"key"});
    redis_.sync_commit();
    int exists = fut.get().as_integer();
    if(exists){
        return false;
    }
    redis_.set(account+"key", password);
    redis_.set(name,account);
    redis_.set(account, name);
    redis_.sync_commit();
    std::string sql1 = "insert into account_info(account,password)values('" +
                       account + "','" + password + "')";
    std::string sql2 =
        "insert into account_online_info(account,online)values('" + account +
        "',0)";
    std::string sql3 = "insert into name_info (name,account)values('" +
                       name + "','" + account + "')";
    std::string sql4 =
        "insert into onlinename_info (name,online)values('" + name + "',0)";
    mysql_.addmsg(sql1.c_str());
    mysql_.addmsg(sql2.c_str());
    mysql_.addmsg(sql3.c_str());
    mysql_.addmsg(sql4.c_str());
    return true;
}
bool Verifycode::verify(std::string account,std::string code) {
    auto fut = redis_.get(account + "code");
    std::string name = getname(account);
    redis_.sync_commit();
    if (fut.get().as_string() == code) {
        redis_.del({account + "code"});
        redis_.sync_commit();
        redis_.set("online" + account, "1");
        redis_.set("onlinename" + name, "1");
        redis_.sync_commit();
        std::string sql =
            "update account_online_info set online=1 where account='" +
            account + "'";
        mysql_.changemsg(sql.c_str());
        sql = "update onlinename_info set online =1 where name ='" + name + "'";
        mysql_.changemsg(sql.c_str());
        return true;
    }
    return false;
}
bool Verifycode::is_online(std::string account) {
    auto fut = redis_.exists({"online" + account});
    redis_.sync_commit();
    if (!fut.get().as_integer()) {
        std::string sql =
            "select online from account_online_info where account ='" +
            account + "'";
        std::string res = mysql_.selectstring(sql.c_str());
        if(res.empty()){
            return false;
        }else if(res=="1"){
            redis_.set("online" + account, "1");
            redis_.sync_commit();
            return true;
        }else{
            return false;
        }
    }else{
        return true;
    }
}
int Verifycode::loginwithkey(std::string account,std::string password) {
    std::string name = getname(account);
    std::string sql =
        "select password from account_info where account='" + account + "'";
    std::string res = mysql_.selectstring(sql.c_str());
    if (!res.empty()) {
        redis_.set(account + "key", res);
        redis_.sync_commit();
    }
    auto fut = redis_.get(account + "key");
    redis_.sync_commit();
    auto reply = fut.get();
    if (!reply.is_string()) {
        return 1;
    }
    std::string hashkey = reply.as_string();
    if(password!=hashkey) {
        return 2;
    }
    redis_.set("online" + account, "1");
    redis_.set("onlinename" + name, "1");
    redis_.sync_commit();
    sql = "update account_online_info set online=1 where account='" + account +
          "'";
    mysql_.changemsg(sql.c_str());
    sql = "update onlinename_info set online =1 where name ='" + name + "'";
    mysql_.changemsg(sql.c_str());
    return 0;
}
bool Verifycode::forgetkey(std::string account) {
    auto fut = redis_.exists({account+"key"});
    redis_.sync_commit();
    if (fut.get().as_integer() == 0) {
        std::string sql = "select password from account_info where account ='"+account+"'";
        std::string res = mysql_.selectstring(sql.c_str());
        if (res.empty()) {
            return false;
        } else {
            redis_.set(account + "key", res);
            redis_.sync_commit();
        }
    }
    addredis(account);
    auto fut1 = redis_.get(account+"key");
    redis_.sync_commit();
    std::string hashcode = fut1.get().as_string();
    sendcom( account, "密码", "您的密码是: " + hashcode);
    return true;
}
int Verifycode::destroy(std::string account,std::string password,Group&group,Friend&f) {
    std::string name = getname(account);
    auto fut = redis_.exists({account + "key"});
    redis_.sync_commit();
    if (fut.get().as_integer() == 0) {
        std::string sql = "select password from account_info";
        std::string res = mysql_.selectstring(sql.c_str());
        if (res.empty()) {
            return 1;
        } else {
            redis_.set(account + "key", res);
            redis_.sync_commit();
        }
    }
    auto fut1 = redis_.get(account+"key");
    redis_.sync_commit();
    auto reply = fut1.get();
    if (reply.as_string() !=password){
        return 2;
    }
    for(auto &t:f.friendlist(account)){
        f.delfriend(account, t);
    }
    for(auto&t:group.grouplist(account)){
        if(group.findowner(t)==account){
            group.delgroup(t, account);
        }else{
            group.delmember(t, account);
        }
    }
    std::string sql =
        "delete from account_info where account='" + account + "'";
    mysql_.delmsg(sql.c_str());
    std::string sql1 =
        "delete from account_online_info where account='" + account + "'";
    mysql_.delmsg(sql1.c_str());
    sql = "delete from apply_info where account ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from applyaddfriend_info where appaccount ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql =
        "delete from applyaddfriend_info where appedaccount ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from block_info where account ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from block_info where blockfriend ='" + account + ";";
    mysql_.delmsg(sql.c_str());
    sql = "delete from disconnectmsg_info where account ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from friend_info where account ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from friend_info where friendaccount ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from groupmanage_info where manager ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from groupmember_info where account ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "select groupname from owner_info where owner ='" + account + "'";
    std::string groupname = mysql_.selectstring(sql.c_str());
    sql = "delete from owner_info where owner ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from groupchat_history where groupname ='" + groupname + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from groupmanage_info where groupname ='" + groupname + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from groupmember_info where groupname ='" + groupname + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from friendchat_history where sender ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from friendchat_history where reciver ='" + account + "'";
    mysql_.delmsg(sql.c_str());
    sql="delete from onlinename_info where name ='"+name+"'";
    mysql_.delmsg(sql.c_str());
    sql = "delete from name_info where name ='" + name + "'";
    mysql_.delmsg(sql.c_str());
    std::cout << "name =" << name << std::endl;
    redis_.del({account + "key", account + "code", "online" + account, account,
                name, "onlinename" + name, "friend" + account,
                "block" + account, "blocked" + account, "addfriend" + account,
                "addedfriend" + account, "grouplist" + account,
                "disconnectmsg" + account});
    redis_.sync_commit();
    return 0;
}
void Verifycode::exitlogin(std::string account){
    redis_.del({"online" + account});
    std::string s = "del online" + account;
    std::cout << s << std::endl;
    redis_.sync_commit();
    std::cout << "redis 删除执行" << std::endl;
    std::string sql =
        "update account_online_info set online=0 where account='" + account +
        "'";
    mysql_.changemsg(sql.c_str());
    std::string name = getname(account);
    sql = "update onlinename_info set online =0 where name ='" + name + "'";
    mysql_.changemsg(sql.c_str());
}
void Verifycode::sendcom(
                         const std::string clientaccount,
                         const std::string& subject,
                         const std::string code) {
    CURL* curl = curl_easy_init();
    
    std::string from = "<" + server + ">";
    std::string to = "<" + clientaccount + ">";
    std::string mail = "To: " + clientaccount +
                       "\r\n"
                       "From: " +
                       server+
                       "\r\n"
                       "Subject: " +
                       subject + "\r\n\r\n" + code;
    struct curl_slist* recipients = NULL;
    recipients = curl_slist_append(recipients, to.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.qq.com:465");
    curl_easy_setopt(curl, CURLOPT_USERNAME, server.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, "miojajsaujebdbch");
    // curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=PLAIN");
    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, mail_payload);
    curl_easy_setopt(curl, CURLOPT_READDATA, &mail);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        std::cout << "发送成功！" << std::endl;
    } else {
        std::cout << "发送失败: " << curl_easy_strerror(res) << std::endl;
    }
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
}
