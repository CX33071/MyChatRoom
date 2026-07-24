#include "Mysql.h"
Mysql :: Mysql() {
    mysql_ = mysql_init(nullptr);
    mysql_real_connect(mysql_, "127.0.0.1", "cx33071", "Caolingxi", "chatroom",
                       3306, nullptr, 0);
    mysql_set_character_set(mysql_, "utf8mb4");
}
void Mysql ::createinfo(const char* sql_){
    mysql_query(mysql_, sql_);
}
void Mysql ::addmsg(const char* sql_){
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
    }
}
void Mysql ::delmsg(const char* sql_){
    mysql_query(mysql_, sql_);
}
void Mysql ::changemsg(const char* sql_){
    mysql_query(mysql_, sql_);
}
std::vector<std::string> Mysql::selectmul(const char* sql_) {
    std::vector<std::string> result;
    mysql_query(mysql_, sql_);
    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        return result;
    }
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        if (row[0]) {
            result.push_back(row[0]);
        }
    }
    mysql_free_result(res);
    return result;
}
std::string Mysql::selectstring(const char* sql_){
    mysql_query(mysql_, sql_);
    MYSQL_RES* res = mysql_store_result(mysql_);
    MYSQL_ROW row = mysql_fetch_row(res);
    std::string result;
    if(row){
        if(row[0]){
            result = row[0];
        }
    }
    mysql_free_result(res);
    return result;
}
bool Mysql::select(const char* sql_) {
    mysql_query(mysql_, sql_);
    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        mysql_free_result(res);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        mysql_free_result(res);
        return false;
    }
    mysql_free_result(res);
    return true;
}
std::vector<std::string> Mysql::selectmul2(const char* sql_) {
    std::vector<std::string> result;
    mysql_query(mysql_, sql_);
    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res){
        return result;
    }
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        result.push_back(std::string(row[0]) + ": " + row[1]);
    }
    mysql_free_result(res);
    return result;
}