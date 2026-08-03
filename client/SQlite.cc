#include <SQlite.h>
SQlite::SQlite():db_(nullptr){
}
SQlite::~SQlite(){
    sqlite3_close(db_);
    db_ = nullptr;
}
void SQlite::open(std::string name){
    std::string path = "./";
    path += name.c_str();
    path += ".db";
    sqlite3_open(path.c_str(), &db_);
    createtable();
}
void SQlite::createtable(){
    std::string sql =
        "create table if not exists friend_chat(id integer primary key "
        "autoincrement,sender varchar(50),reciver varchar(50),content "
        "text,send_time datetime default current_timestamp);";
    sqlite3_exec(db_,sql.c_str(),nullptr,nullptr,nullptr);
    sql =
        "create table if not exists group_chat (id integer primary key "
        "autoincrement,groupname varchar(50),sender varchar(50),content "
        "text,send_time datetime default current_timestamp);";
    sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
}
void SQlite::addfriendchat(std::string sender,
                  std::string reciver,
                  std::string text) {
    std::string sql =
        "insert into friend_chat (sender,reciver,content)values('" + sender +
        "','" + reciver + "','" + text + "');";
    char* e;
    int res = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &e);

    if (res != SQLITE_OK) {
        std::cout << "SQL执行失败："<<e<<std::endl;
        sqlite3_free(e);
    }
}
void SQlite::addgroupchat(std::string groupname,
                          std::string sender,
                          std::string text){
    std::string sql =
        "insert into group_chat (groupname,sender,content)values('" +
        groupname + "','" + sender + "','" + text + "');";
    sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
}
std::vector<std::string> SQlite::getfriendchat(std::string account,
                                               std::string friendaccount){
    std::string sql = "select sender,content from friend_chat where  (sender='" +
                      account + "'and reciver ='" + friendaccount + "')or (sender ='"+friendaccount+"'and reciver ='"+account+"')order by send_time";
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr)!=SQLITE_OK){
        std::cout << "SQ执行失败" << sqlite3_errmsg(db_) << std::endl;
    }

    std::vector<std::string> res;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string s1 =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string s2 =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string s = "[" + s1 + "]:" + s2;
        res.push_back(s);
    }
    return res;
}
std::vector<std::string> SQlite::getgroupchat(std::string groupname) {
    std::string sql =
        "select sender,content from group_chat where groupname ='" + groupname + "'order by send_time;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    std::vector<std::string> res;
    while(sqlite3_step(stmt)==SQLITE_ROW){
        std::string t1 =
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string s2=reinterpret_cast<const char*>(sqlite3_column_text(stmt,1));
        std::string s = "[" + t1 + "]:" + s2;
        res.push_back(s);
    }
    return res;
}