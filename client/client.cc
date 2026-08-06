#pragma once
#include <termios.h>
#include <wait.h>
#include <condition_variable>
#include <future>
#include "ChatClient.h"
#include "FileClient.h"
#include <queue>
#include <chrono>
#include <unordered_map>
#include "/home/cx33071/muduo-/net/TcpClient.h"
#include "json.hpp"
#include <signal.h>
#define RESET "\033[0m"
#define GREEN "\033[1;32m"
#define BLUEC "\033[1;34m"
#define BLUE "\033[34m"
#define PURPLE "\033[1;35m"
using json = nlohmann::json;
std::atomic<bool> stop =false;
std::atomic<bool> friendmenu=false;
std::atomic<bool> groupmenu=false;
std::atomic<bool> filemenu = false;
std::atomic<bool> msgmenu = false;
EventLoop* L;
chatclient* c;
termios old_term;
std::string get_current_time() {
    Timestamp now = Timestamp::now();
    return now.toFormattedString(true);
}
void handler(int) {
    stop = true;
    c->stop1 = true;
    c->handle_exit();
}
void handle_message(chatclient*client) {
    while (client->running) {
        std::unique_lock<std::mutex> lock(client->msg_mutex);
        client->msg_cv.wait(lock, [&client] { return !client->msg_map.empty()||!client->running; });
        if(!client->running){
            break;
        }
        for (auto it = client->msg_map.begin(); it != client->msg_map.end();) {
            std::string cmd = it->first;
            std::queue<json>& q = it->second;
            while (!q.empty()) {
                json msg = q.front();
                q.pop();
                if(cmd=="finish"){
                    json j1;
                    j1["cmd"] = "sendfinishtofileclient";
                    std::string request_id = msg.value("request_id1", "");
                    if (!request_id.empty()) {
                        j1["request_id"] = request_id;
                    }
                    client->fileclient_->file_conn->send(j1.dump() + '\n');
                } else if (cmd == "chatedres") {
                    chatclient::Event e;
                    e.type = chatclient::Event::FRIENDCHAT;
                    std::string from = msg["account"];
                    std::string content = msg["message"];
                    std::string name1 = client->getname(from);
                    std::string name2 = client->name;
                    std::string q =
                        "\n收到来自[" + name1 + "]的消息:" + content;
                    e.data["data"]=q;
                    e.data["time"] = msg["time"];
                    client->SQ.addfriendchat(name1, name2, content);
                    if (client->inchat && client->current_chat == from) {
                        std::cout << "\r\33[2K";
                        std::cout << "[" << name1 << "]:   " << content
                                  << std::endl;
                        std::cout << GREEN << "[" << name2
                                  << "]:    " << RESET;
                        std::cout.flush();

                    } else {
                        if(!client->is_blockfriend(from)){
                            {
                                std::lock_guard<std::mutex> lock(client->event_mutex);
                                client->event_queue.push(e);
                                client->event_cv.notify_one();
                            }
                        }
                    }
                } else if (cmd == "groupchatedres") {
                    chatclient::Event e;
                    e.type = chatclient::Event::GROUPCHAT;
                    std::string from = msg["account"];
                    std::string name = client->getaccount(from);
                    std::string content = msg["message"];
                    std::string groupname = msg["groupname"];
                   std::string content1 = "\n收到来自群聊[" + groupname + "]的成员:[" + name +
                          "]的消息:" + content;
                    e.data["data"] = content1;
                    e.data["time"] = msg["time"];
                    client->SQ.addgroupchat(groupname, name, content);
                    if (client->groupchat) {
                        std::cout << "\r\33[2K";
                        std::cout << "[" << name << "]:   " << content
                                  << std::endl;
                        std::cout << GREEN << "[" << client->name
                                  << "]:    " << RESET;
                        std::cout.flush();
                    } else {
                        {
                            std::lock_guard<std::mutex> lock(client->event_mutex);
                            client->event_queue.push(e);
                            client->event_cv.notify_one();
                        }
                    }
                } else if (cmd == "addedres") {
                    chatclient::Event e;
                    e.type = chatclient::Event::Friendadd;
                    e.data = msg;
                    std::string from = msg["target"] ;
                   json s;
                   s["account"] = from;
                   s["time"] = e.data["time"];
                   client->addlist.push_back(s);
                   if (!client->is_blockfriend(from)) {
                       {
                           std::lock_guard<std::mutex> lock(
                               client->event_mutex);
                           client->event_queue.push(e);
                           client->event_cv.notify_one();
                       }
                    }
                } else if (cmd == "toRETRres") {
                    std::cout << PURPLE << msg["data"] <<msg["time"]<< RESET << std::endl;
                } else if (cmd == "sendedfile") {
                    chatclient::Event e;
                    e.type = chatclient::Event::SENDFILE;
                    e.data = msg;
                    std::string from = msg["from"];
                    json apply;
                    apply["from"] = from;
                    apply["filename"] = e.data["filename"];
                    apply["ID"] = e.data["ID"];
                    apply["data"] = e.data["data"];
                    
                    client->sendfilelist.push_back(apply);
                    if (!client->is_blockfriend(from)) {
                        {
                            std::lock_guard<std::mutex> lock(client->event_mutex);
                            client->event_queue.push(e);
                            client->event_cv.notify_one();
                        }
                    }
                }else if(cmd =="groupsendedfile"){
                    chatclient::Event e;
                    e.type = chatclient::Event::SENDFILE;
                    e.data = msg;
                    std::string from = msg["from"];
                    json apply;
                    apply["from"] = from;
                    
                    apply["filename"] = e.data["filename"];
                    apply["ID"] = e.data["ID"];
                    apply["data"] = e.data["data"];
                    client->sendfilelist.push_back(apply);
                    if (!client->is_blockfriend(from)) {
                        {
                            std::lock_guard<std::mutex> lock(
                                client->event_mutex);
                            client->event_queue.push(e);
                            client->event_cv.notify_one();
                        }
                    }
                } else if (cmd == "appliedjoinres") {
                    chatclient::Event e;
                    e.type = chatclient::Event::APPLYJOINGROUP;
                    e.data = msg;
                    std::string from = e.data["account"];
                    std::string t = get_current_time();
                    json apply;
                    apply["from"] = from;
                    apply["groupname"] = e.data["groupname"];
                    apply["data"] = e.data["data"];
                    client->applyjoinlist.push_back(apply);
                    if (!client->is_blockfriend(from)) {
                        {
                            std::lock_guard<std::mutex> lock(client->event_mutex);
                            client->event_queue.push(e);
                            client->event_cv.notify_one();
                        }
                    }
                } else {
                    std::string data = msg["data"];
                    std::cout <<PURPLE<< "\n[系统消息]: " << data<<RESET << std::endl;
                }
            }
            if (q.empty()) {
                it = client->msg_map.erase(it);
            } else {
                ++it;
            }
        }
    }
}
void handle_event(chatclient*client) {
    while (client->running) {
        std::unique_lock<std::mutex> lock(client->event_mutex);
        client->event_cv.wait(lock, [
            &client] { return !client->event_queue.empty()||!client->running; });
        if(!client->running){
            break;
        }
        chatclient::Event e;
        e = client->event_queue.front();
        client->event_queue.pop();
        json reply;
        if (e.type == chatclient::Event::Friendadd) {
            std::cout << std::endl;
            std::cout << PURPLE << e.data["time"] << "[系统消息]："
                      << e.data["message"] << "请稍后在菜单中查看" << "\n"
                      << "请继续菜单输入:" << RESET << std::endl;
        }else if(e.type==chatclient::Event::SENDFILE){
            std::cout << std::endl;
            std::cout << PURPLE << "[系统消息]："
                      << e.data["data"] << "请稍后在菜单中查看" << "\n"
                      << "请继续菜单输入:" << RESET << std::endl;
        }
         else if (e.type == chatclient::Event::GroupInvite) {
            std::cout << std::endl;
            std::cout << PURPLE << e.data["time"]
                      << "[系统消息]:" << e.data["data"] << "请稍候在菜单中查看"
                      << "\n"
                      << "请继续菜单输入:" << RESET << std::endl;
        }else if(e.type==chatclient::Event::FRIENDCHAT){
            std::cout << std::endl;
            std::cout << PURPLE<<e.data["time"] << "[系统消息]:" << e.data["data"]
                      << "请稍候在菜单中查看" << "\n"
                      << "请继续菜单输入:" << RESET << std::endl;
        } else if (e.type == chatclient::Event::GROUPCHAT) {
            std::cout << std::endl;
            std::cout << PURPLE<<e.data["time"] << "[系统消息]:" << e.data["data"]
                      << "请稍候在菜单中查看" << "\n"
                      << "请继续菜单输入:" << RESET << std::endl;
        }else if(e.type==chatclient::Event::APPLYJOINGROUP){
            std::cout << std::endl;
            std::cout << PURPLE <<e.data["time"]<< "\n[系统消息]:" << e.data["data"]
                      << "请稍候在菜单中查看:" << "\n"
                      << "请继续菜单输入:" << RESET << std::endl;
        }
    }
}
void mainfunction(chatclient*chatclient) {
    json res;
    while (chatclient->running) {
        if (!chatclient->chatis_login) {
            chatclient->main_menu();
            int choice;
            std::string s;
            char* line = readline(GREEN "请选择:" RESET);
            if(line==nullptr){
                chatclient->stop1 = true;
                if(chatclient->chatis_login){
                    chatclient->handle_exitlogin();
                }
                chatclient->handle_exit();
                break;
            }
            s = line;
            free(line);
            choice=chatclient->changenum(s);
            while(choice<0||choice>5){
                if (choice == -1) {
                    line = readline(GREEN "请输入数字:" RESET);
                    if (line == nullptr) {
                        chatclient->stop1 = true;
                        if (chatclient->chatis_login) {
                            chatclient->handle_exitlogin();
                        }
                        chatclient->handle_exit();
                        break;
                    }
                    s = line;
                    free(line);
                    choice = chatclient->changenum(s);
                }else{
                    line = readline(GREEN "请输入数字0-5:" RESET);
                    if (line == nullptr) {
                        chatclient->stop1 = true;
                        if (chatclient->chatis_login) {
                            chatclient->handle_exitlogin();
                        }
                        chatclient->handle_exit();
                        break;
                    }
                    s = line;
                    free(line);
                    choice = chatclient->changenum(s);
                }
            }
            switch (choice) {
                case 1:
                    chatclient->handle_signup();
                    break;
                case 2:
                    chatclient->handle_login_code();
                    break;
                case 3:
                    chatclient->handle_login_key();
                    break;
                case 4:
                    chatclient->handle_forget_key();
                    break;
                case 5:
                    chatclient->handle_destory();
                    break;
                case 0:
                    chatclient->handle_exitlogin();
                    chatclient->handle_exit();
                    break;
                default:
                    std::cout<<PURPLE << "请输入有效选项!" <<RESET<< std::endl;
                    break;
            };
        } else {
            if(friendmenu){
                chatclient->friend_menu();
                chatclient->handle_onlinelist();
                chatclient->handle_blocklist();
                int choice;
                std::string s;
                char* line = readline(GREEN "请选择:" RESET);
                if (line == nullptr) {
                    chatclient->stop1 = true;
                    if (chatclient->chatis_login) {
                        chatclient->handle_exitlogin();
                    }
                    chatclient->handle_exit();
                    break;
                }
                s = line;
                free(line);
                choice = chatclient->changenum(s);
                while (choice < 0 || choice > 11) {
                    if (choice == -1) {
                        line = readline(GREEN "请输入数字:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    } else {
                        line = readline(GREEN "请输入数字0-11:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    }
                }
                switch (choice) {
                            case 1:
                                chatclient->handle_addfriend();
                                break;
                            case 2:
                                chatclient->handle_delfriend();
                                break;
                            case 3:
                                chatclient->handle_block();
                                break;
                            case 4:
                                chatclient->handle_disblock();
                                break;
                            case 5:
                                chatclient->handle_friendlist();
                                break;
                            case 0:
                                chatclient->handle_exitlogin();
                                chatclient->handle_exit();
                                break;
                            case 6:
                                chatclient->handle_blocklist();
                                break;
                            case 7:
                                chatclient->handle_addfriendmsg();
                                break;
                            case 8:
                                chatclient->handle_friendchat();
                                break;
                            case 9:
                                chatclient->handle_onlinelist();
                                break;
                            case 10:
                                friendmenu = false;
                                groupmenu = false;
                                filemenu = false;
                                msgmenu = false;
                                break;
                            case 11:
                                chatclient->handle_exitlogin();
                                break;
                            default:
                                std::cout << PURPLE << "请输入有效选项!"
                                          << RESET << std::endl;
                                break;
                        };
            } else if(msgmenu){
                chatclient->msg_menu();
                int choice;
                std::string s;
                char* line = readline(GREEN "请选择:" RESET);
                if (line == nullptr) {
                    chatclient->stop1 = true;
                    if (chatclient->chatis_login) {
                        chatclient->handle_exitlogin();
                    }
                    chatclient->handle_exit();
                    break;
                }
                s = line;
                free(line);
                choice = chatclient->changenum(s);
                while (choice < 0 || choice > 5) {
                    if (choice == -1) {
                        line = readline(GREEN "请输入数字:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    } else {
                        line = readline(GREEN "请输入数字0-5:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    }
                }
                switch (choice) {
                    case 1:
                        chatclient->handle_addfriendmsg();
                        break;
                    case 2:
                        chatclient->handle_applyjoinmsg();
                        break;
                    case 3:
                        chatclient->handle_sendedfile();
                        break;
                    case 4:
                        friendmenu = false;
                        groupmenu = false;
                        filemenu = false;
                        msgmenu = false;
                        break;
                    case 5:
                        chatclient->handle_exitlogin();
                        break;
                    case 0:
                        chatclient->handle_exitlogin();
                        chatclient->handle_exit();
                        break;
                    default:
                        std::cout << PURPLE << "请输入有效选项!" << RESET
                                  << std::endl;
                        break;
                };
            } else if (groupmenu) {
                chatclient->group_menu();
                chatclient->handle_ownergrouplist();
                chatclient->handle_grouplist();
                int choice;
                std::string s;
                char* line = readline(GREEN "请选择:" RESET);
                if (line == nullptr) {
                    chatclient->stop1 = true;
                    if (chatclient->chatis_login) {
                        chatclient->handle_exitlogin();
                    }
                    chatclient->handle_exit();
                    break;
                }
                s = line;
                free(line);
                choice = chatclient->changenum(s);
                while (choice < 0 || choice > 12) {
                    if (choice == -1) {
                        line = readline(GREEN "请输入数字:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    } else {
                        line = readline(GREEN "请输入数字0-12:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    }
                }
                switch (choice) {
                    case 1:
                        chatclient->handle_creategroup();
                        break;
                    case 2:
                        chatclient->handle_applyjoingroup();
                        break;
                    case 3:
                        chatclient->handle_exitgroup();
                        break;
                    case 4:
                        chatclient->handle_groupmember();
                        break;
                    case 5:
                        chatclient->handle_setgroupmanager();
                        break;
                    case 0:
                        chatclient->handle_exitlogin();
                        chatclient->handle_exit();
                        break;
                    case 6:
                        chatclient->handle_delgroup();
                        break;
                    case 7:
                        chatclient->handle_grouplist();
                        break;
                    case 8:
                        chatclient->handle_delmember();
                        break;
                    case 9:
                        chatclient->handle_groupchat();
                        break;
                    case 10:
                        chatclient->handle_applyjoinmsg();
                        break;
                    case 11:
                        friendmenu = false;
                        groupmenu = false;
                        filemenu = false;
                        msgmenu = false;
                        break;
                    case 12:
                        chatclient->handle_exitlogin();
                        break;
                    default:
                        std::cout << PURPLE << "请输入有效选项!" << RESET
                                  << std::endl;
                        break;
                };
            } else if (filemenu) {
                chatclient->file_menu();
                int choice;
                std::string s;
                char* line = readline(GREEN "请选择:" RESET);
                if (line == nullptr) {
                    chatclient->stop1 = true;
                    if (chatclient->chatis_login) {
                        chatclient->handle_exitlogin();
                    }
                    chatclient->handle_exit();
                    break;
                }
                s = line;
                free(line);
                choice = chatclient->changenum(s);
                while (choice < 0 || choice > 6) {
                    if (choice == -1) {
                        line = readline(GREEN "请输入数字:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    } else {
                        line = readline(GREEN "请输入数字0-6:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    }
                }
                switch (choice) {
                    case 1:
                        chatclient->handle_sendfile();
                        break;
                    case 2:
                        chatclient->handle_sendedfile();
                        break;
                    case 3:
                        chatclient->handle_groupsendfile();
                        break;
                    case 5:
                        friendmenu = false;
                        groupmenu = false;
                        filemenu = false;
                        msgmenu = false;
                        break;
                    case 0:
                        chatclient->handle_exitlogin();
                        chatclient->handle_exit();
                        break;
                    case 6:
                        chatclient->handle_exitlogin();
                        break;
                    case 4:
                        chatclient->handle_uploadingfile();
                        break;
                    default:
                        std::cout << PURPLE << "请输入有效选项!" << RESET
                                  << std::endl;
                        break;
                };
            } else {
                chatclient->select_menu();
                int choice;
                std::string s;
                char* line = readline(GREEN "请选择:" RESET);
                if (line == nullptr) {
                    chatclient->stop1 = true;
                    if (chatclient->chatis_login) {
                        chatclient->handle_exitlogin();
                    }
                    chatclient->handle_exit();
                    break;
                }
                s = line;
                free(line);
                choice = chatclient->changenum(s);
                while (choice < 0 || choice > 5) {
                    if (choice == -1) {
                        line = readline(GREEN "请输入数字:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    } else {
                        line = readline(GREEN "请输入数字0-5:" RESET);
                        if (line == nullptr) {
                            chatclient->stop1 = true;
                            if (chatclient->chatis_login) {
                                chatclient->handle_exitlogin();
                            }
                            chatclient->handle_exit();
                            break;
                        }
                        s = line;
                        free(line);
                        choice = chatclient->changenum(s);
                    }
                }
                switch (choice) {
                    case 1:
                        friendmenu = true;
                        break;
                    case 2:
                        groupmenu = true;
                        break;
                    case 3:
                        filemenu = true;
                        break;
                    case 4:
                        msgmenu = true;
                        break;
                    case 5:
                        chatclient->handle_exitlogin();
                        break;
                    case 0:
                        chatclient->handle_exitlogin();
                        chatclient->handle_exit();
                        break;
                    default:
                        std::cout << PURPLE << "请输入有效选项!" << RESET
                                  << std::endl;
                        break;
                };
            }
        }
        
        }
    }
    long isport(char* s) {
        char* end;
        errno = 0;
        long port = std::strtol(s, &end, 10);
        if(errno==ERANGE){
            return 2;
        }
        if (*end != '\0') {
            return 1;
        }
        if (port <= 0 || port > 65535) {
            return 2;
        }
        
        return 0;
    }
int main(int argc, char* argv[]) {
    if(argc!=4){
        std::cout << PURPLE << "请输入./client (ip) (chat_port) (file_port)"
                  << RESET << std::endl;
        return -1;
    }
    long res1 = isport(argv[2]);
    long res2 = isport(argv[3]);
    if (res1 == 1 || res2 == 1) {
        std::cout << PURPLE << "port请输入数字!" << RESET << std::endl;
        return -1;
    }
    if (res1 == 2 || res2 == 2) {
        std::cout << PURPLE << "port范围为1~65535!" << RESET << std::endl;
        return -1;
    }
    rl_catch_signals = 0;
    signal(SIGINT, handler);
    signal(SIGTSTP, handler);
    EventLoop loop;
    L = &loop;
    InetAddress chataddr(argv[1], std::stoi(argv[2]));
    InetAddress fileaddr(argv[1], std::stoi(argv[3]));
    chatclient chatclient(&loop, chataddr);
    c = &chatclient;
    FileClient fileclient(&loop, fileaddr);
    chatclient.setfileclient(&fileclient);
    fileclient.setchatclient(&chatclient);
    chatclient.running = true;
    std::thread t1(mainfunction, &chatclient);
    std::thread t2(handle_message,&chatclient);
    std::thread t3(handle_event,&chatclient);
    int timeout = -1;
    loop.loop(timeout);
    if (stop) {
        chatclient.stop1 = true;
        _exit(0);
    } else {
        if (t1.joinable()) {
            t1.join();
        }
        if (t2.joinable()) {
            t2.join();
        }
        if (t3.joinable()) {
            t3.join();
        }
    }
    return 0;
}