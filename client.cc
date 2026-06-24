#pragma once
#include "choicework.h"
Choicework choicework;
std::unordered_map<std::string, std::queue<json>> msg_map;
std::string get_current_time(){
    Timestamp now = Timestamp::now();
    return now.toFormattedString(true);
}
void connectioncallback(const TcpClient::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        c_conn = conn;
        connok = true;
        c_cv.notify_all();
    } else {
        c_conn.reset();
        connok = false;
    }
}
void messagecallback(const TcpClient::TcpConnectionPtr& conn,
                     Buffer* buf,
                     Timestamp) {
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
        if(!id.empty()){
            std::lock_guard<std::mutex> lock(active_mutex);
            auto it = active_requests.find(id);
            if (it != active_requests.end()) {
                it->second.set_value(j);
                active_requests.erase(it);
            }
            continue;
        }
        std::string cmd = j["cmd"];
        {
            std::lock_guard<std::mutex> lock(msg_mutex);
            msg_map[cmd].push(j);
            msg_cv.notify_all();
        }
    }
}
void handle_message() {
    while (1) {
        std::unique_lock<std::mutex> lock(msg_mutex);
        msg_cv.wait(lock, [] { return !msg_map.empty(); });
        for (auto it = msg_map.begin(); it != msg_map.end();) {
            std::string cmd = it->first;
            std::queue<json>& q = it->second;
            while (!q.empty()) {
                json msg = q.front();
                q.pop();
                if (cmd == "chat" || cmd == "groupchat") {
                    std::string from = msg["from"];
                    std::string content = msg["message"];
                    std::cout << "\n收到来自" << from << "的消息: " << content
                              << std::endl;
                } else if (cmd == "addedres") {
                    Event e;
                    e.type = Event::Friendadd;
                    e.data = msg;
                    std::string t = get_current_time();
                    std::string from = msg["target"] ;
                    addlist.push_back(from+" 消息时间："+t);
                    {
                        std::lock_guard<std::mutex> lock(event_mutex);
                        event_queue.push(e);
                        event_cv.notify_one();
                    }
                } else if (cmd == "invitedres") {
                    Event e;
                    e.type = Event::GroupInvite;
                    e.data = msg;
                    std::string from = e.data;
                    std::string t = get_current_time();
                    invitelist.push_back(from+" 消息时间："+t);
                    {
                        std::lock_guard<std::mutex> lock(event_mutex);
                        event_queue.push(e);
                        event_cv.notify_one();
                    }
                } else if (cmd == "keysignin_res") {
                    std::string code = msg["code"];

                    if (code == "1") {
                        is_login = true;
                    }
                    std::string data = msg["data"];
                    std::cout << PURPLE<<"\n[系统消息]: " << data << RESET<<std::endl;
                } else if (cmd == "codesignin_res") {
                    std::string code = msg["code"];
                    if (code == "1") {
                        is_login = true;
                    }
                    std::string data = msg["data"];
                    std::cout << "\n[系统消息]: " << data << std::endl;
                } else {
                    std::string data = msg["data"];
                    std::cout <<PURPLE<< "\n[系统消息]: " << data<<RESET << std::endl;
                }
            }
            if (q.empty()) {
                it = msg_map.erase(it);
            } else {
                ++it;
            }
        }
    }
}
void handle_event() {
    while (1) {
        std::unique_lock<std::mutex> lock(event_mutex);
        event_cv.wait(lock, [] { return !event_queue.empty(); });
        Event e;
        e = event_queue.front();
        event_queue.pop();
        json reply;
        if (e.type == Event::Friendadd) {
            std::cout << PURPLE<<"\n[系统消息]：" << e.data["message"]
                      << "请稍后在菜单中查看" << "\n"
                      << "请继续菜单输入:" <<RESET<< std::endl;
        } else if (e.type == Event::GroupInvite) {
            std::cout << PURPLE<<"\n[系统消息]:" << e.data["data"]
                      << "请稍候在菜单中查看" << "\n"
                      << "请继续菜单输入:"<<RESET <<std::endl;
        }
    }
}

void mainfunction() {
    json res;
    while (1) {
        if (!is_login) {
            choicework.main_menu();
        } else {
            choicework.friend_menu();
        }
        int choice;
        std::string sschoice;
        std::getline(std::cin >> std::ws, sschoice);
        choice = std::stoi(sschoice);
        switch (choice) {
            case 1:
                choicework.handle_signup();
                break;
            case 2:
                choicework.handle_login_code();
                break;
            case 3:
                choicework.handle_login_key();
                break;
            case 4:
                choicework.handle_forget_key();
                break;
            case 5:
                choicework.handle_destory();
                break;
            case 0:
                choicework.handle_exit();
                return;
            case 6:
                choicework.handle_addfriend();
                break;
            case 7:
                choicework.handle_friendlist();
                break;
            case 8:
                choicework.handle_friendchat();
                break;
            case 9:
                choicework.handle_block();
                break;
            case 10:
                choicework.handle_delfriend();
                break;
            case 11:
                choicework.handle_creategroup();
                break;
            case 12:
                choicework.handle_invite();
                break;
            case 13:
                choicework.handle_exitgroup();
                break;
            case 14:
                choicework.handle_groupchat();
                break;
            case 15:
                is_login = false;
                break;
            case 16:
                choicework.handle_addfriendmsg();
                break;
            case 17:
                choicework.handle_invitemsg();
                break;
            case 18:
                choicework.handle_exit();
                return;
            default:
                std::cout << choice;
                std::cout << "请输入有效选项!" << std::endl;
                break;
        };
    }
}
int main(int argc, char* argv[]) {
    EventLoop loop;
    InetAddress addr(argv[1], 8888);
    TcpClient client(&loop, addr);
    client.setConnectionCallback(connectioncallback);
    client.setMessageCallback(messagecallback);
    client.connect();
    std::thread t1(mainfunction);
    std::thread t2(handle_message);
    std::thread t3(handle_event);
    int timeout = -1;
    loop.loop(timeout);
    msg_cv.notify_all();
    event_cv.notify_all();
    t2.join();
    t3.join();
    return 0;
}