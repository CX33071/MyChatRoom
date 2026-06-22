#include <termios.h>
#include <wait.h>
#include <condition_variable>
#include <future>
#include <queue>
#include "/home/cx33071/muduo-/net/TcpClient.h"
#include "json.hpp"
using json = nlohmann::json;
TcpClient::TcpConnectionPtr c_conn;
std::condition_variable c_cv;
std::mutex c_mutex;
std::mutex msg_mutex;
bool connok = false;
bool is_login = false;
std::atomic<bool> recive{false};
bool friendrecive = false;
bool is_addchoice = false;
bool input=false;
bool input_ok = false;
enum Inputstatus {
    menu,
    event_confirm,
    none,
    chat,
    cinaccount,
    cinkey,
    cincode,
    cinnum,
    cinyn,
    cinfriend,
    cingroupname
};

Inputstatus status=Inputstatus::menu;
struct Inputbag{
    std::string content;
    Inputstatus s;
};
std::queue<Inputbag> input_queue;
std::mutex input_mutex;
std::condition_variable input_cv;
std::unordered_map<std::string, std::queue<json>> msg_map;
std::condition_variable msg_cv;
int choice;
std::string num;
std::string account;
std::string friendaccount;
std::string code;
std::string key;
std::string ss;
std::string groupname;
std::string friendtarget;
struct Event {
    enum Type { FriendRequest, GroupInvite } type;
    json data;
};
std::queue<Event> event_queue;
std::mutex event_mutex;
std::atomic<bool> running{true};
std::condition_variable event_cv;
std::string inputkey() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::string key;
    std::cin >> key;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return key;
}
bool startinput = false;
void input_thread() {
    while (1) {
        if(startinput){
            std::string line;
            if (status == Inputstatus::cinkey) {
                line = inputkey();
            } else {
                std::getline(std::cin, line);
            }
            {
                std::lock_guard<std::mutex> lock(input_mutex);
                while (!input_queue.empty())
                    input_queue.pop();
                input_queue.push({line, status});
                input_cv.notify_one();
            }
            startinput = false;
        }
    }
}
std::string waitinputbase(Inputstatus currentstatus) {
    status = currentstatus;
    std::unique_lock<std::mutex> lock(input_mutex);
    startinput = true;
    input_cv.wait(lock, [] { return !input_queue.empty() || !running; });
    auto input = input_queue.front();
    input_queue.pop();
    return input.content;
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
        std::string cmd = j["cmd"];
        {
            std::lock_guard<std::mutex> lock(msg_mutex);
            msg_map[cmd].push(j);
            msg_cv.notify_all();
        }
    }
}

void handle_message() {
    while (running) {
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
                    std::cout << "\n收到来自" << from << "的消息: " << content << std::endl;
                } else if (cmd == "addedres") {
                    Event e;
                    e.type = Event::FriendRequest;
                    e.data = msg;
                    {
                        std::lock_guard<std::mutex> lock(event_mutex);
                        event_queue.push(e);
                    }
                } else if (cmd == "invitedres") {
                    Event e;
                    e.type = Event::GroupInvite;
                    e.data = msg;
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
                    std::cout << "\n[系统消息]: " << data <<std::endl;
                } else if (cmd == "codesignin_res") {
                    std::string code = msg["code"];
                    if (code == "1") {
                        is_login = true;
                    }
                    std::string data = msg["data"];
                    std::cout << "\n[系统消息]: " << data << std::endl;
                } else {
                    std::string data = msg["data"];
                   
                    std::cout << "\n[系统消息]: " << data << std::endl;
                }
                recive = true;
            }
            if (q.empty()) {
                it = msg_map.erase(it);
            } else {
                ++it;
            }
        }
    }
}


bool if_event(Event& e){
    std::lock_guard<std::mutex> lock(event_mutex);
    if(event_queue.empty()){
        return false;
    }
    e = event_queue.front();
    event_queue.pop();
    return true;
}
void handle_event(Event e){
    json reply;
    if (e.type == Event::FriendRequest) {
        std::cout << "\n好友申请:" << e.data["message"] << '\n'
                  << "请输入是否同意y/n: " << std::endl;
        std::string choice = waitinputbase(Inputstatus::event_confirm);
        if (choice == "y" || choice == "Y") {
            reply["cmd"] = "agreefriend";
            std::cout << "已同意好友申请" << std::endl;
        } else {
            reply["cmd"] = "refusefriend";
            std::cout << "已经拒绝好友申请" << std::endl;
        }
        reply["account"] = account;
        reply["friendaccount"] = e.data["target"];
        c_conn->send(reply.dump() + '\n');
        status = Inputstatus::menu;

    } else if (e.type == Event::GroupInvite) {
        std::cout << "\n群邀请:" << e.data["data"] << std::endl;
        std::cout << "请输入是否同意y/n: ";
        std::string choice = waitinputbase(Inputstatus::event_confirm);
        if (choice == "y" || choice == "Y") {
            reply["cmd"] = "agreejoin";
            std::cout << "已同意入群申请" << std::endl;
        } else {
            reply["cmd"] = "refusegroup";
            std::cout << "已经拒绝入群申请" << std::endl;
        }
        reply["account"] = account;
        reply["groupname"] = e.data["groupname"];
        reply["target"] = e.data["account"];
        c_conn->send(reply.dump() + '\n');
        status = Inputstatus::menu;
    }
}
void main_menu() {
    std::cout << "\n";
    std::cout << "欢迎使用MyChatRoom!" << std::endl;
    std::cout << "    用户管理\n";
    std::cout << "1.注册\n";
    std::cout << "2.验证码登录\n";
    std::cout << "3.密码登录\n";
    std::cout << "4.忘记密码\n";
    std::cout << "5.注销账号\n";
    std::cout << "0.退出\n";
    std::cout << "请选择:" << std::endl;
    ;
}
void friend_menu() {
    std::cout << "\n好友管理\n";
    std::cout << "6.添加好友\n";
    std::cout << "7.好友列表\n";
    std::cout << "8.私聊\n";
    std::cout << "9.拉黑好友\n";
    std::cout << "10.删除好友\n";
    std::cout << "11.创建群聊\n";
    std::cout << "12.邀请好友加入群聊\n";
    std::cout << "13.删除群聊\n";
    std::cout << "14.群聊天\n";
    std::cout << "15. 返回主菜单\n";
    std::cout << "请选择:" << std::endl;
    ;
}

void mainfunction() {
    std::string password;
    std::string verifycode;
    json j;
    std::string frienduser;
    std::string servermsg;
    std::string chatmsg;
    std::string groupname;
    std::string groupmsg;
    std::string ss;
    json res;
    while (1) {
        Event e;
        while(if_event(e)){
            handle_event(e);
        }
        if (!is_login) {
            main_menu();
        } else {
            friend_menu();
        }
        while (if_event(e)) {
            handle_event(e);
        }
        status = Inputstatus::menu;
        std::string input = waitinputbase(Inputstatus::menu);
        int choice = std::stoi(input);
        switch (choice) {
            case 1:
                if (is_login) {
                    std::cout << "请重新输入6-15之间数字!" << std::endl;
                    break;
                }
                std::cout << "请输入你的qq邮箱:";
                account = waitinputbase(Inputstatus::cinaccount);
                std::cout << "请输入你的密码:";
                status = Inputstatus::cinkey;
                password = waitinputbase(Inputstatus::cinkey);
                std::cout << std::endl;
                j["cmd"] = "signup";
                j["account"] = account;
                j["password"] = password;
                c_conn->send(j.dump() + '\n');
                std::cout << "注册请求已经发送!" << std::endl;
                recive = false;
                while (!recive) {
                }
                recive = false;
                break;
            case 2:
                if (is_login) {
                    std::cout << "请重新输入6-15之间数字!" << std::endl;
                    break;
                }
                std::cout << "请先输入你的qq邮箱:";
                account = waitinputbase(Inputstatus::cinaccount);
                j["cmd"] = "verifycode";
                j["account"] = account;
                c_conn->send(j.dump() + '\n');
                std::cout << "验证码已经发送到您的qq邮箱\n";
                std::cout << "请输入验证码:";
                verifycode = waitinputbase(Inputstatus::cincode);
                j["cmd"] = "verifycodesignin";
                j["account"] = account;
                j["code"] = verifycode;
                c_conn->send(j.dump() + '\n');
                recive = false;
                while (!recive) {
                }
                recive = false;
                break;
            case 3:
                if (is_login) {
                    std::cout << "请重新输入6-15之间数字!" << std::endl;
                    break;
                }
                std::cout << "请先输入你的qq邮箱:"<<std::endl;
                account = waitinputbase(Inputstatus::cinaccount);
               
                std::cout << "请输入您的密码:";
                status=Inputstatus::cinkey;
                password = waitinputbase(Inputstatus::cinkey);
                std::cout << std::endl;
                j["cmd"] = "keysignin";
                j["account"] = account;
                j["password"] = password;
                c_conn->send(j.dump() + '\n');
                while (!recive) {
                }
                recive = false;
                break;
            case 4:
                if (is_login) {
                    std::cout << "请重新输入6-15之间数字!" << std::endl;
                    break;
                }
                std::cout << "请输入你的qq邮箱: ";
                status = Inputstatus::cinaccount;
                input = true;
                while (!input_ok) {
                }
                input_ok = false;

                j["cmd"] = "forgetkey";
                j["account"] = account;
                c_conn->send(j.dump() + '\n');
                std::cout << "密码已经发送到您的qq邮箱！" << std::endl;
                while (!recive) {
                }
                break;
            case 5:
                if (is_login) {
                    std::cout << "请重新输入6-15之间数字!" << std::endl;
                    break;
                }
                std::cout << "请输入您的密码:";
                password = inputkey();
                j["cmd"] = "destory";
                j["account"] = account;
                j["password"] = password;
                c_conn->send(j.dump() + '\n');
                break;
            case 0:
                if (is_login) {
                    std::cout << "请重新输入6-15之间数字!" << std::endl;
                    break;
                }
                    std::cout << "再见！" << std::endl;
                    running = false;
                    msg_cv.notify_all();
                    event_cv.notify_all();
                    return;
                
            case 6:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                std::cout << "请输入要添加好友的账号:";
                frienduser = waitinputbase(Inputstatus::cinfriend);
                j["cmd"] = "addfriend";
                j["from"] = account;
                j["to"] = frienduser;
                c_conn->send(j.dump() + '\n');
                while(!recive){}
                break;
            case 7:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                j["cmd"] = "friendlist";
                j["account"] = account;
                c_conn->send(j.dump() + '\n');
                break;
            case 8: 
            case 9:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                std::cout << "请输入要拉黑的好友账号:";
                status = Inputstatus::cinfriend;
                input = true;
                while (!input_ok) {
                }
                input_ok = false;

                j["cmd"] = "block";
                j["account"] = account;
                j["target"] = frienduser;
                c_conn->send(j.dump() + '\n');
                break;
            case 10:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                std::cout << "请输入要删除的好友账号:";
                status = Inputstatus::cinfriend;
                input = true;
                while (!input_ok) {
                }
                input_ok = false;

                j["cmd"] = "delfriend";
                j["account"] = account;
                j["target"] = frienduser;
                c_conn->send(j.dump() + '\n');
                break;
            case 11:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                std::cout << "请输入要创建的群聊的名字:";
                status = Inputstatus::cingroupname;
                input = true;
                while (!input_ok) {
                }
                input_ok = false;

                j["cmd"] = "creategroup";
                j["account"] = account;
                j["groupname"] = groupname;
                c_conn->send(j.dump() + '\n');
                break;
            case 12:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                std::cout << "请输入要邀请好友加入的群聊名称:";
                status = Inputstatus::cingroupname;
                input = true;
                while (!input_ok) {
                }
                input_ok = false;

                std::cout << "请输入要邀请哪位好友加入该群聊:";
                status = Inputstatus::cinfriend;
                input = true;
                while (!input_ok) {
                }
                input_ok = false;

                j["cmd"] = "invite";
                j["account"] = account;
                j["groupname"] = groupname;
                j["target"] = frienduser;
                c_conn->send(j.dump() + '\n');
                std::cout << "邀请已经发送，等待对方确认" << std::endl;
                break;
            case 13:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                std::cout << "请输入要删除的群聊的名称:";
                status = Inputstatus::cingroupname;
                input = true;
                while (!input_ok) {
                }
                input_ok = false;

                j["cmd"] = "delgroup";
                j["account"] = account;
                j["groupname"] = groupname;
                c_conn->send(j.dump() + '\n');
                break;
            case 14:
            case 15:
                if (!is_login) {
                    std::cout << "请先登录!" << std::endl;
                    break;
                }
                is_login = false;
                std::cout << "已返回主菜单" << std::endl;
                break;

            default:

                std::cout << "请输入有效选项!" << std::endl;
                exit(0);
                break;
        };
        input = false;
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
    std::thread t4(input_thread);
    int timeout = -1;
    loop.loop(timeout);
    msg_cv.notify_all();
    event_cv.notify_all();
    t2.join();
    return 0;
}