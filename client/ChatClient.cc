#include "ChatClient.h"
#include "FileClient.h"
chatclient::chatclient(EventLoop* loop, const InetAddress& addr):client_(loop,addr),loop_(loop){
    client_.setConnectionCallback(
        [this](const TcpClient::TcpConnectionPtr& conn) { connectioncallback(conn); });

    client_.setMessageCallback(
        [this](const TcpClient::TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
            messagecallback(conn, buf, t);
        });
    client_.connect();
    loop_->runEvery(30, [this]() { sendheart(); });
}
void chatclient::setfileclient(FileClient*client){
    fileclient_ = client;
}
void chatclient::connectioncallback(const TcpClient::TcpConnectionPtr& conn){
    if (conn->connected()) {
        chat_conn = conn;
        chatconnok = true;
    } else {
        chat_conn.reset();
        chatconnok = false;
        chatis_login = false;
        std::cout << "\n服务器断开连接\n";
    }
}
void chatclient::sendheart(){
    if(!chat_conn){
        return;
    }
    json j1;
    j1["cmd"] = "heart";
    chat_conn->send(j1.dump() + '\n');
}
void chatclient::messagecallback(const TcpClient::TcpConnectionPtr& conn,
                     Buffer* buf,
                     Timestamp){
    while (1) {
        const char* pos = buf->findn();
        if (!pos) {
            break;
        }
        std::string msg(buf->peek(), pos - buf->peek());
        buf->retrieveUntil(pos + 1);

        json j;
        j = json::parse(msg);
        id = j.value("request_id", "");
        if (!id.empty()) {
            std::lock_guard<std::mutex> lock(active_mutex);
            auto it = active_requests.find(id);
            if (it != active_requests.end()) {
                it->second.set_value(j);
                active_requests.erase(it);
            }
        } else {
            std::string cmd = j["cmd"];
            {
                std::lock_guard<std::mutex> lock(msg_mutex);
                msg_map[cmd].push(j);
                msg_cv.notify_all();
            }
        }
    }
}
std::string chatclient::cinkey() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::string key;
    std::getline(std::cin >> std::ws, key);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return key;
}
std::string chatclient::gen_req_id() {
    return std::to_string(++req_id);
}
void chatclient::main_menu() {
    std::cout << "\n";
    std::cout << GREEN << "————————————欢迎使用MyChatRoom!——————————" << RESET << std::endl;
    std::cout << GREEN << "================用户管理================\n" << RESET;
    std::cout << GREEN << "1.               注册               \n" << RESET;
    std::cout << GREEN << "2.           验证码登录               \n" << RESET;
    std::cout << GREEN << "3.            密码登录               \n" << RESET;
    std::cout << GREEN << "4.            忘记密码               \n" << RESET;
    std::cout << GREEN << "5.            注销账号               \n" << RESET;
    std::cout << GREEN << "0.               退出               \n" << RESET;
    std::cout << GREEN << "请选择:" << RESET;
}
void chatclient::friend_menu() {
    std::cout << GREEN << "\n================好友功能================\n"
              << RESET;
    std::cout << GREEN << "6.              添加好友               \n" << RESET;
    std::cout << GREEN << "7.              删除好友               \n" << RESET;
    std::cout << GREEN << "8.              拉黑好友               \n" << RESET;
    std::cout << GREEN << "9.              取消拉黑               \n" << RESET;
    std::cout << GREEN << "10.             好友列表               \n" << RESET;
    std::cout << GREEN << "11.           拉黑好友列表               \n"
              << RESET;
    std::cout << GREEN << "12.          处理好友申请消息               \n"
              << RESET;
    std::cout << GREEN << "13.              私聊               \n" << RESET;
    std::cout << GREEN << "14.          显示好友在线状态               \n"
              << RESET;
    std::cout << GREEN << "\n================群聊功能================\n"
              << RESET;
    std::cout << GREEN << "15.             创建群聊               \n"
              << RESET;
    std::cout << GREEN << "16.           申请加入群聊               \n"
              << RESET;
    std::cout << GREEN << "17.             退出群聊               \n" << RESET;
    std::cout << GREEN << "18.            查看群聊成员               \n"
              << RESET;
    std::cout << GREEN << "19.           设置群聊管理员               \n"
              << RESET;
    std::cout << GREEN << "20.             解散群聊               \n"
              << RESET;
    std::cout << GREEN << "21.             群聊列表               \n"
              << RESET;
    std::cout << GREEN << "22.            移除群聊成员               \n" << RESET;
    std::cout << GREEN << "23.             群聊天               \n" << RESET;
    std::cout << GREEN << "24.           处理加群申请               \n"
              << RESET;
    std::cout << GREEN << "\n================文件功能================\n";
    std::cout << GREEN << "25.           好友上传文件               \n" << RESET;
    std::cout << GREEN << "26.           处理文件消息               \n" << RESET;
    std::cout << GREEN << "27.           群聊上传文件               \n"
              << RESET;
    std::cout << GREEN << "28.            退出登录               \n"
              << RESET;
    std::cout << GREEN << "29.          退出chatroom               \n"
              << RESET;
    std::cout << GREEN << "请选择:" << RESET;
}

void chatclient::handle_signup() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        std::cout << "请输入你的qq邮箱:";
        std::getline(std::cin >> std::ws, account);
        std::cout << "请输入你的密码:";
        password = cinkey();
        std::cout << std::endl;
        j["cmd"] = "signup";
        j["account"] = account;
        j["password"] = password;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}

void chatclient::handle_login_code() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        std::cout << "请先输入你的qq邮箱:";
        std::getline(std::cin >> std::ws, account);
        j["cmd"] = "verifycode";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }

        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        if (res["code"] == "0") {
            std::cout << PURPLE << res["data"] << RESET << std::endl;
            return;
        }
        std::cout << "验证码已经发送到您的qq邮箱\n";
        std::cout << "请输入验证码:";
        std::cin >> verifycode;
        j["cmd"] = "verifycodesignin";
        j["account"] = account;
        j["code"] = verifycode;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p2;
        std::future<json> f2 = p2.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p2);
        }

        chat_conn->send(j.dump() + '\n');
        res = f2.get();
        std::string cc = res["code"];

        if (cc == "1") {
            chatis_login = true;
        }
        std::cout << res["data"] << std::endl;
    }
}
void chatclient::handle_login_key() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        std::cout << PURPLE << "请先输入你的qq邮箱:" << RESET;
        std::getline(std::cin >> std::ws, account);
        std::cout << PURPLE << "请输入您的密码:" << RESET;
        password = cinkey();
        j["cmd"] = "keysignin";
        j["account"] = account;
        j["password"] = password;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::string cc = res["code"];

        if (cc == "1") {
            chatis_login = true;
        }
        std::cout << res["data"] << std::endl;
    }
}

void chatclient::handle_forget_key() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        std::cout << "请输入你的qq邮箱: ";
        std::getline(std::cin >> std::ws, account);
        j["cmd"] = "forgetkey";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void chatclient::handle_destory() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        std::cout << PURPLE << "请先输入你的qq邮箱:" << RESET;
        std::getline(std::cin >> std::ws, account);
        std::cout << PURPLE << "请输入您的密码:" << RESET;
        password = cinkey();
        j["cmd"] = "destory";
        j["account"] = account;
        j["password"] = password;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}

void chatclient::handle_exit() {
    std::cout << "再见！" << std::endl;
    running = false;
    msg_cv.notify_all();
    event_cv.notify_all();
    chatis_login = false;
}
void chatclient::handle_addfriend() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << PURPLE << "请输入要添加好友的账号:" << RESET;
        std::getline(std::cin >> std::ws, frienduser);
        int is = is_friend(frienduser);
        if (is == 1) {
            std::cout << "该账号并不存在" << std::endl;
        } else if (is == 2) {
            std::cout << "该用户已经是您的好友" << std::endl;
        } else {
            j["cmd"] = "addfriend";
            j["from"] = account;
            j["to"] = frienduser;
            id = gen_req_id();
            j["request_id"] = id;
            std::promise<json> p;
            std::future<json> f = p.get_future();
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_requests[id] = std::move(p);
            }
            chat_conn->send(j.dump() + '\n');
            json res = f.get();
            std::cout << PURPLE << res["data"] << RESET << std::endl;
        }
    }
}
void chatclient::handle_applyjoingroup() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << PURPLE << "请输入您要加入的群聊名称:" << RESET;
        std::getline(std::cin >> std::ws, groupname);
        bool b = is_existsgroup(groupname);
        if (!b) {
            std::cout << "该群聊并不存在!" << std::endl;
            return;
        }
        b = is_groupmember(groupname, account);
        if (b) {
            std::cout << "您已经是群聊成员了" << std::endl;
            return;
        }
        j["cmd"] = "applyjoingroup";
        j["account"] = account;
        j["groupname"] = groupname;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}
void chatclient::handle_exitlogin() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        json j1;
        j1["cmd"] = "exitlogin";
        j1["account"] = account;
        id = gen_req_id();
        j1["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j1.dump() + '\n');
        json res = f.get();
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}
int chatclient::is_friend(std::string friendaccount) {
    j["cmd"] = "is_friend";
    j["account"] = account;
    j["friendaccount"] = friendaccount;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res = f.get();
    std::string historystring = res["data"];
    size_t start = 0;
    size_t end = historystring.find('\n');
    while (end != std::string ::npos) {
        friendhistory.push_back(historystring.substr(start, end - start));
        start = end + 1;
        end = historystring.find('\n', start);
    }
    if (start < historystring.length()) {
        friendhistory.push_back(historystring.substr(start));
    }
    if (res["code"] == "1") {
        return 1;
    } else if (res["code"] == "2") {
        return 2;
    } else {
        return 0;
    }
}
void chatclient::handle_friendchat() {
    std::cout << "请输入要私聊的好友账号:";
    std::getline(std::cin >> std::ws, current_chat);
    friendhistory.clear();
    int is = is_friend(current_chat);
    if (is == 1) {
        std::cout << "该账号并不存在" << std::endl;
    } else if (is == 0) {
        std::cout << "对方目前还不是您的好友" << std::endl;
    } else {
        inchat = true;
        system("clear");
        std::cout << GREEN << "进入与用户[" << current_chat << "]的聊天界面"
                  << RESET << std::endl;
        for (int i = 0; i < friendhistory.size(); i++) {
            std::cout << friendhistory[i] << std::endl;
        }
        while (inchat) {
            std::string msg;
            std::cout << GREEN "[" + account + "]:   " << RESET;
            std::getline(std::cin >> std::ws, msg);
            if (msg == "EXIT") {
                inchat = false;
                break;
            }
            json chat;
            chat["cmd"] = "friendchat";
            chat["from"] = account;
            chat["to"] = current_chat;
            chat["message"] = msg;
            chat_conn->send(chat.dump() + '\n');
        }
    }
}
void chatclient::handle_friendlist() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        j["cmd"] = "friendlist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << PURPLE << data << RESET << std::endl;
    }
}
void chatclient::printfmembers() {
    j["cmd"] = "groupmember";
    j["account"] = account;
    j["groupname"] = groupname;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res = f.get();
    size_t pos = 0;
    std::string data = res["data"];
    while ((pos = data.find("\\n", pos)) != std::string::npos) {
        data.replace(pos, 2, "\n");
        pos += 1;
    }
    std::cout << PURPLE << data << RESET << std::endl;
}
bool chatclient::is_groupmember(std::string groupname, std::string count) {
    j["cmd"] = "is_groupmember";
    j["groupname"] = groupname;
    j["account"] = count;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res = f.get();
    if (res["code"] == "1") {
        return true;
    } else {
        return false;
    }
}
bool chatclient::is_manager(std::string groupname, std::string count) {
    j["cmd"] = "is_manager";
    j["account"] = count;
    j["groupname"] = groupname;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res = f.get();
    if (res["code"] == "1") {
        return true;
    } else {
        return false;
    }
}
bool chatclient::is_existsgroup(std::string groupname) {
    j["cmd"] = "is_existsgroup";
    j["groupname"] = groupname;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res = f.get();
    if (res["code"] == "1") {
        return true;
    } else {
        return false;
    }
}
void chatclient::handle_setgroupmanager() {
    if (!chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        std::cout << "请输入要管理的群聊名称:";
        std::getline(std::cin >> std::ws, groupname);
        bool b = is_existsgroup(groupname);
        if (!b) {
            std::cout << "该群聊并不存在" << std::endl;
            return;
        }
        b = is_groupmember(groupname, account);
        if (!b) {
            std::cout << "您并不在该群当中，无管理权限" << std::endl;
            return;
        }
        b = is_manager(groupname, account);
        if (!b) {
            std::cout << "您并不是该群群主或者管理员，无管理权限" << std::endl;
            return;
        }
        printfmembers();
        std::cout << "请输入要添加管理员1 / 删除管理员2:";
        int num;
        std::cin >> num;
        if (num == 1) {
            std::string count;
            std::cout << "请输入要添加为管理员的用户帐号:";
            std::getline(std::cin >> std::ws, count);
            b = is_exists(count);
            if(!b){
                std::cout << PURPLE << "该用账号并不存在" << RESET << std::endl;
            }
            b = is_groupmember(groupname, count);
            if (!b) {
                std::cout << "该用户并不是该群成员，请先邀请该用户进群"
                          << std::endl;
                return;
            }
            b = is_manager(groupname, count);
            if (b) {
                std::cout << "该用户已经是管理员了" << std::endl;
                return;
            }
            j["cmd"] = "addmanager";
            j["groupname"] = groupname;
            j["account"] = count;
        } else {
            std::string count;
            std::cout << "请输入要删除的管理员帐号:";
            std::getline(std::cin >> std::ws, count);
            b = is_groupmember(groupname, count);
            if (!b) {
                std::cout << "该用户并不是该群成员，请先邀请该用户进群"
                          << std::endl;
                return;
            }
            b = is_manager(groupname, count);
            if (!b) {
                std::cout << "该用户并不是管理员身份" << std::endl;
                return;
            }
            j["cmd"] = "delmanager";
            j["groupname"] = groupname;
            j["account"] = count;
        }
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}
void chatclient::handle_groupmember() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "请输入要查看群成员的群聊名称:";
        std::getline(std::cin >> std::ws, groupname);
        printfmembers();
    }
}
void chatclient::handle_grouplist() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        j["cmd"] = "grouplist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << PURPLE << data << RESET << std::endl;
    }
}
void chatclient::handle_blocklist() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        j["cmd"] = "blocklist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << PURPLE << data << RESET << std::endl;
    }
}
void chatclient::handle_onlinelist() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        j["cmd"] = "onlinelist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << PURPLE << data << RESET << std::endl;
    }
}
void chatclient::handle_block() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "请输入要拉黑的好友账号:";
        std::getline(std::cin >> std::ws, frienduser);
        int is = is_blockfriend(frienduser);
        if (is == 1) {
            std::cout << "该账号并不存在" << std::endl;
        } else if (is == 2) {
            std::cout << "该用户已经在您的拉黑名单当中" << std::endl;
        } else {
            j["cmd"] = "block";
            j["account"] = account;
            j["target"] = frienduser;
            id = gen_req_id();
            j["request_id"] = id;
            std::promise<json> p;
            std::future<json> f = p.get_future();
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_requests[id] = std::move(p);
            }
            chat_conn->send(j.dump() + '\n');
            json res = f.get();
            std::cout << res["data"] << std::endl;
        }
    }
}
int chatclient::is_blockfriend(std::string friendaccount) {
    json j1;
    j1["cmd"] = "is_block";
    j1["account"] = account;
    j1["friendaccount"] = friendaccount;
    id = gen_req_id();
    j1["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j1.dump() + '\n');
    json res = f.get();
    if (res["data"] == "1") {
        return 1;
    } else if (res["data"] == "2") {
        return 2;
    } else {
        return 0;
    }
}
bool chatclient::is_exists(std::string account){
    j["cmd"]="isexists";
    j["account"] = "account";
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res = f.get();
    if(res["code"]=="1"){
        return true;
    }
    return false;
}
void chatclient::handle_disblock() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "请输入要取消拉黑的好友账号:";
        std::getline(std::cin >> std::ws, frienduser);
        bool b = is_exists(frienduser);
        if(!b){
            std::cout << PURPLE << "该账号并不存在" << RESET << std::endl;
            return;
        }
        int is = is_blockfriend(frienduser);
        if (is == 1) {
            std::cout << "该账号并不存在" << std::endl;
        } else if (is == 0) {
            std::cout << "该用户并没有在您的拉黑名单当中" << std::endl;
        } else {
            j["cmd"] = "cancleblock";
            j["account"] = account;
            j["target"] = frienduser;
            id = gen_req_id();
            j["request_id"] = id;
            std::promise<json> p;
            std::future<json> f = p.get_future();
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_requests[id] = std::move(p);
            }
            chat_conn->send(j.dump() + '\n');
            json res = f.get();
            std::cout << PURPLE << res["data"] << RESET << std::endl;
        }
    }
}
void chatclient::handle_delfriend() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << PURPLE << "请输入要删除的好友账号:" << RESET;
        std::getline(std::cin >> std::ws, frienduser);
        json s;
        s["cmd"] = "delfriend";
        s["account"] = account;
        s["target"] = frienduser;
        id = gen_req_id();
        s["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(s.dump() + '\n');
        json res = f.get();
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}
void chatclient::handle_creategroup() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "请输入要创建的群聊的名字:";
        std::getline(std::cin >> std::ws, groupname);
        j["cmd"] = "creategroup";
        j["account"] = account;
        j["groupname"] = groupname;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void chatclient::handle_exitgroup() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "请输入要退出的群聊的名称:";
        std::getline(std::cin >> std::ws, groupname);
        j["cmd"] = "exitgroup";
        j["account"] = account;
        j["groupname"] = groupname;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}
void chatclient::handle_delmember() {
    std::cout << PURPLE << "请输入您要删除成员的群聊名称:" << RESET;
    std::getline(std::cin >> std::ws, groupname);
    bool b = is_existsgroup(groupname);
    if (!b) {
        std::cout << "该群聊并不存在" << std::endl;
        return;
    }
    b = is_manager(groupname, account);
    if (!b) {
        std::cout << "您并不是该群群主或者管理员，没有移出群成员的权限"
                  << std::endl;
        return;
    }
    printfmembers();

    std::cout << PURPLE << "请输入您要删除的成员:" << RESET;
    std::string count;
    std::getline(std::cin >> std::ws, count);
    b = is_groupmember(groupname, count);
    if (!b) {
        std::cout << "该用户并不是群聊成员" << std::endl;
        return;
    }
    j["cmd"] = "delmember";
    j["account"] = count;
    j["groupname"] = groupname;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res = f.get();
    std::cout << PURPLE << res["data"] << RESET << std::endl;
}
void chatclient::handle_delgroup() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "请输入要解散的群聊名称:";
        std::getline(std::cin >> std::ws, groupname);
        std::cout << PURPLE << "请输入您的密码:" << RESET;
        password = cinkey();
        j["cmd"] = "delgroup";
        j["account"] = account;
        j["groupname"] = groupname;
        j["password"] = password;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void chatclient::handle_addfriendmsg() {
    json reply;
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        if(addlist.empty()){
            std::cout << PURPLE << "当前没有需要处理的好友申请消息哦!" << RESET
                      << std::endl;
            return;
        }else{
            std::cout << "好友申请消息：" << std::endl;
            for (auto i = 0; i < addlist.size(); i++) {
                std::cout << GREEN << "[" << i + 1
                          << "]:" << addlist[i]["account"]
                          << "消息时间:" << addlist[i]["time"] << RESET
                          << std::endl;
            }
            std::cout << "请选择要处理的消息编号:";
            int num;
            std::cin >> num;
            int total = addlist.size();
            while(num>total){
                std::cout << PURPLE << "请输入正确消息编号哦!" << RESET
                          << std::endl;
                std::cin >> num;
            }
            frienduser = addlist[num - 1]["account"];
            std::string ss;
            std::cout << std::endl;
            std::cout << "请输入y 同意|n 拒绝:";
            std::getline(std::cin >> std::ws, ss);
            num -= 1;
            while(true){
                if (ss == "y" || ss == "Y") {
                    reply["cmd"] = "agreefriend";
                    std::cout << "已同意好友申请" << std::endl;
                    break;
                } else if (ss == "n" || ss == "N") {
                    reply["cmd"] = "refusefriend";
                    std::cout << "已经拒绝好友申请" << std::endl;
                    break;
                } else {
                    std::cout << PURPLE << "请输入y 同意|n 拒绝:";
                    std::getline(std::cin >> std::ws, ss);
                }
            }
            addlist.erase(addlist.begin() + num);
            reply["account"] = account;
            reply["friendaccount"] = frienduser;
            id = gen_req_id();
            reply["request_id"] = id;
            std::promise<json> p;
            std::future<json> f = p.get_future();
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_requests[id] = std::move(p);
            }
            chat_conn->send(reply.dump() + '\n');
            json res = f.get();
            std::cout << PURPLE << "[系统消息]:" << res["data"] << RESET
                      << std::endl;
        }
       
    }
}
void chatclient::handle_sendedfile() {
    json reply;
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
       if(sendfilelist.empty()){
           std::cout << PURPLE << "当前并没有文件要处理哦!" << RESET
                     << std::endl;
           return;
       }else{
           std::cout << GREEN << "好友发送文件消息：" << RESET << std::endl;
           for (auto i = 0; i < sendfilelist.size(); i++) {
               std::cout << GREEN << "[" << i + 1 << "]"
                         << sendfilelist[i]["data"] << RESET << std::endl;
           }
           std::cout << GREEN << "请选择要处理的消息编号:";
           int num;
           std::cin >> num;
           while (num > sendfilelist.size()) {
               std::cout << PURPLE << "请输入正确消息编号" << RESET
                         << std::endl;
               std::cin >> num;
           }
           std::cout << PURPLE << "请输入您要下载文件到本地的路径：" << RESET;
           std::getline(std::cin >> std::ws, filepath);
           std::cout << RESET << std::endl;
           std::string from = sendfilelist[num - 1]["from"];
         
           filename = sendfilelist[num - 1]["filename"];
           
               reply["cmd"] = "recvfile";

           reply["from"] = from;
           reply["ID"] = sendfilelist[num - 1]["ID"];
           std::string ID = sendfilelist[num - 1]["ID"];
           reply["filename"] = filename;
           id = gen_req_id();
           reply["request_id"] = id;
           std::promise<json> p;
           std::future<json> f = p.get_future();
           {
               std::lock_guard<std::mutex> lock(active_mutex);
               active_requests[id] = std::move(p);
           }
           chat_conn->send(reply.dump() + '\n');
           sendfilelist.erase(sendfilelist.begin() + num - 1);
           json res = f.get();
           std::string filesize = res["filesize"];
           fileclient_->loadfile(filename, filesize, ID, filepath);
           std::cout << PURPLE << "文件下载完成" << RESET << std::endl;
       }
    }
}
void chatclient::handle_applyjoinmsg() {
    json reply;
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
       if(applyjoinlist.empty()){
           std::cout << PURPLE << "当前并没有入群申请通知" << RESET
                     << std::endl;
           return;
       }else{
           std::cout << "申请加入群聊消息：" << std::endl;
           for (auto i = 0; i < applyjoinlist.size(); i++) {
               std::cout << GREEN << "[" << i + 1 << "]"
                         << applyjoinlist[i]["data"] << RESET << std::endl;
           }
           std::cout << "请选择要处理的消息编号:";
           int num;
           std::cin >> num;
           while (num > applyjoinlist.size()) {
               std::cout << PURPLE << "请输入正确的编号" << RESET << std::endl;
               std::cin >> num;
           }
           std::cout << std::endl;
           std::string g = applyjoinlist[num - 1]["groupname"];
           std::string a = applyjoinlist[num - 1]["from"];
           std::cout << "请处理来自用户:" << a << "加入群聊" << g
                     << "的消息:" << std::endl;
           std::cout << "请输入y 同意|n 拒绝:";
           std::string ss;
           std::getline(std::cin >> std::ws, ss);
           applyjoinlist.erase(applyjoinlist.begin() + num - 1);
           while (true) {
               if (ss == "y" || ss == "Y") {
                   reply["cmd"] = "agreejoingroup";
                   break;
               } else if (ss == "n" || ss == "N") {
                   reply["cmd"] = "refusejoingroup";
                   break;
               } else {
                   std::cout << "输入错误!请输入y 同意|n 拒绝:";
                   std::getline(std::cin >> std::ws, ss);
               }
           }
           reply["account"] = a;
           reply["groupname"] = g;
           id = gen_req_id();
           reply["request_id"] = id;
           std::promise<json> p;
           std::future<json> f = p.get_future();
           {
               std::lock_guard<std::mutex> lock(active_mutex);
               active_requests[id] = std::move(p);
           }
           chat_conn->send(reply.dump() + '\n');
           json res = f.get();
           std::cout << PURPLE << "[系统消息]:" << res["data"] << RESET
                     << std::endl;
       }
    }
}
void chatclient::getgrouphistory(std::string groupname) {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        j["cmd"] = "getgrouphistory";
        j["groupname"] = groupname;
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::string historystring = res["data"];
        size_t start = 0;
        size_t end = historystring.find('\n');
        while (end != std::string ::npos) {
            groupnamehistory.push_back(
                historystring.substr(start, end - start));
            start = end + 1;
            end = historystring.find('\n', start);
        }
        if (start < historystring.length()) {
            groupnamehistory.push_back(historystring.substr(start));
        }
    }
}
void chatclient::handle_groupchat() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << PURPLE << "请输入要进行群聊的群聊名称:" << RESET;
        std::getline(std::cin >> std::ws, groupname);
        bool b = is_existsgroup(groupname);
        if (!b) {
            std::cout << PURPLE << "该群聊并不存在!" << RESET << std::endl;
            return;
        }
        b = is_groupmember(groupname, account);
        if (!b) {
            std::cout << PURPLE << "您当前并不在该群聊里，不能进行聊天" << RESET
                      << std::endl;
            return;
        }
        groupchat = true;
        system("clear");
        std::cout << GREEN << "欢迎进入群聊[" << groupname << "]的聊天界面"
                  << RESET << std::endl;
        std::cout << GREEN << "输入 EXIT 退出当前聊天" << RESET << std::endl;
        getgrouphistory(groupname);
        for (int i = 0; i < groupnamehistory.size(); i++) {
            std::cout << groupnamehistory[i] << std::endl;
        }
        while (groupchat) {
            std::string msg;
            std::cout << GREEN << "[" << account << "]:" << RESET;
            std::getline(std::cin >> std::ws, msg);
            if (msg == "EXIT") {
                groupchat = false;
                break;
            }
            json chat;
            chat["cmd"] = "groupchat";
            chat["account"] = account;
            chat["groupname"] = groupname;
            chat["message"] = msg;
            chat_conn->send(chat.dump() + '\n');
        }
    }
}

void chatclient::handle_sendfile() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "请输入您要发送文件的用户帐号:";
        std::getline(std::cin >> std::ws, frienduser);
        int b = is_friend(frienduser);
        if (b == 1) {
            std::cout << "该用户账号不存在" << std::endl;
            return;
        }
        if (b == 0) {
            std::cout << "该用户目前还不是您的好友，不能传文件" << std::endl;
            return;
        }
        std::cout << "请输入您要上传的文件路径:";
        std::getline(std::cin >> std::ws, filepath);
        int fd = open(filepath.c_str(), O_RDONLY);
        if (fd == -1) {
            std::cout << "文件打开失败" << std::endl;
            return;
        }
        struct stat st;

        if (stat(filepath.c_str(), &st) == -1) {
            std::cout << "文件不存在\n";
            return;
        }

        uint64_t filesize = st.st_size;
        close(fd);
        char buf[256];
        strcpy(buf, filepath.c_str());
        filename = basename(buf);
        json j;
        j["cmd"] = "sendfile";
        j["from"] = account;
        j["to"] = frienduser;
        j["filename"] = filename;
        j["filesize"] = std::to_string(filesize);
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::string ID = res["ID"];
        fileclient_->sendfile(ID, filepath, filename, std::to_string(filesize));
    }
}
void chatclient::handle_groupsendfile(){
    std::cout<<PURPLE<<"请输入您要发送文件群聊名称:";
    std::string groupname;
    std::getline(std::cin>>std::ws,groupname);
    bool b= is_existsgroup(groupname);
    if(!b){
        std::cout << PURPLE << "该群聊并不存在" << RESET << std::endl;
        return;
    }else if(!is_groupmember(groupname,account)){
        std::cout << PURPLE << "你并不在该群聊里" << RESET << std::endl;
        return;
    }else{
        std::cout << PURPLE << "请输入你要上传文件的路径:" << RESET
                  << std::endl;
        std::getline(std::cin >> std::ws, filepath);
        int fd = open(filepath.c_str(), O_RDONLY);
        if (fd == -1) {
            std::cout << "文件打开失败" << std::endl;
            return;
        }
        struct stat st;

        if (stat(filepath.c_str(), &st) == -1) {
            std::cout << "文件不存在\n";
            return;
        }

        uint64_t filesize = st.st_size;
        close(fd);
        char buf[256];
        strcpy(buf, filepath.c_str());
        filename = basename(buf);
        json j;
        j["cmd"] = "groupsendfile";
        j["from"] = account;
        j["groupname"] = groupname;
        j["filename"] = filename;
        j["filesize"] = std::to_string(filesize);
        id = gen_req_id();
        j["request_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        chat_conn->send(j.dump() + '\n');
        json res = f.get();
        std::string ID = res["ID"];
        fileclient_->groupsendfile(ID, filepath, filename, std::to_string(filesize));
    }
}
void chatclient::handle_loadfile() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
    }
}