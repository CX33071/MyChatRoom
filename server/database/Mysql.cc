#include "Mysql.h"

namespace {
void init_mysql_thread() {
    struct ThreadGuard {
        ThreadGuard() { mysql_thread_init(); }
        ~ThreadGuard() { mysql_thread_end(); }
    };
    thread_local ThreadGuard guard;
}
}

Mysql::Mysql() {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        std::cout << "mysql init failed" << std::endl;
        return;
    }

    if (!mysql_real_connect(mysql_, "127.0.0.1", "cx33071", "Caolingxi", "chatroom",
                            3306, nullptr, 0)) {
        std::cout << "mysql connect error: " << mysql_error(mysql_) << std::endl;
        mysql_close(mysql_);
        mysql_ = nullptr;
        return;
    }

    if (mysql_set_character_set(mysql_, "utf8mb4")) {
        std::cout << "mysql charset error: " << mysql_error(mysql_) << std::endl;
    }
}

Mysql::~Mysql() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
}

void Mysql::createinfo(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    if (!mysql_) {
        return;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
    }
}

void Mysql::addmsg(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    if (!mysql_) {
        return;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
    }
}

void Mysql::delmsg(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    if (!mysql_) {
        return;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
    }
}

void Mysql::changemsg(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    if (!mysql_) {
        return;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
    }
}

std::vector<std::string> Mysql::selectmul(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    std::vector<std::string> result;
    if (!mysql_) {
        return result;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
        return result;
    }

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

std::string Mysql::selectstring(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    std::string result;
    if (!mysql_) {
        return result;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
        return result;
    }

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        return result;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) {
        result = row[0];
    }
    mysql_free_result(res);
    return result;
}

bool Mysql::select(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    if (!mysql_) {
        return false;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
        return false;
    }

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    bool found = row != nullptr;
    mysql_free_result(res);
    return found;
}

std::vector<std::string> Mysql::selectmul2(const char* sql_) {
    std::lock_guard<std::mutex> lock(mutex_);
    init_mysql_thread();
    std::vector<std::string> result;
    if (!mysql_) {
        return result;
    }
    if (mysql_query(mysql_, sql_)) {
        std::cout << "mysql error: " << mysql_error(mysql_) << std::endl;
        return result;
    }

    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        return result;
    }
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::string first = row[0] ? row[0] : "";
        std::string second = row[1] ? row[1] : "";
        result.push_back(first + ": " + second);
    }
    mysql_free_result(res);
    return result;
}
