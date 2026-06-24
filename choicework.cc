#include "choicework.h"
std::string account;
std::string password;
std::string verifycode;
 std::string frienduser;
 std::string groupname;
 json j;
 TcpClient::TcpConnectionPtr c_conn;
 std::condition_variable c_cv;
 std::mutex c_mutex;
 std::mutex msg_mutex;
 std::condition_variable msg_cv;
 std::mutex active_mutex;
 std::unordered_map<std::string, std::promise<json>> active_requests;
 std::atomic<bool> is_login{false};
 std::atomic<bool> recive{false};
 std::atomic<bool> connok{false};
 std::atomic<bool> running{false};
 std::queue<Event> event_queue;
 std::mutex event_mutex;
 std::condition_variable event_cv;
 std::vector<std::string> addlist;
 std::vector<json> invitelist;
 std::string id;
 std::atomic<int> req_id{0};

 std::string Choicework::cinkey() {
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
std::string Choicework::gen_req_id(){
    return std::to_string(++req_id);
}
void Choicework::main_menu() {
    std::cout << "\n";
    std::cout << GREEN<<"欢迎使用MyChatRoom!"<<RESET << std::endl;
    std::cout << GREEN << "    用户管理\n"<<RESET;
    std::cout << GREEN<<"1.注册\n"<<RESET;
    std::cout << GREEN<<"2.验证码登录\n"<<RESET;
    std::cout <<GREEN<< "3.密码登录\n"<<RESET;
    std::cout <<GREEN<< "4.忘记密码\n"<<RESET;
    std::cout << GREEN<<"5.注销账号\n"<<RESET;
    std::cout << GREEN<<"0.退出\n"<<RESET;
    std::cout << GREEN<<"请选择:"<<RESET;
}
void Choicework::friend_menu() {
    std::cout << GREEN<<"\n好友管理\n"<<RESET;
    std::cout << GREEN << "6.添加好友\n" << RESET;
    std::cout << GREEN << "7.好友列表\n" << RESET;
    std::cout << GREEN << "8.私聊\n" << RESET;
    std::cout << GREEN << "9.拉黑好友\n" << RESET;
    std::cout << GREEN << "10.删除好友\n" << RESET;
    std::cout << GREEN << "11.创建群聊\n" << RESET;
    std::cout << GREEN << "12.邀请好友加入群聊\n" << RESET;
    std::cout << GREEN << "13.删除群聊\n" << RESET;
    std::cout << GREEN << "14.群聊天\n" << RESET;
    std::cout << GREEN << "15. 返回主菜单\n" << RESET;
    std::cout << GREEN << "16.查看好友申请消息\n" << RESET;
    std::cout << GREEN<<"17.查看邀请入群消息\n"<<RESET;
    std::cout << GREEN << "18.退出\n" << RESET;
    std::cout << GREEN << "请选择:" << RESET;
}

void Choicework::handle_signup() {

    if (is_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    }else{
        std::cout << "请输入你的qq邮箱:";
        std::getline(std::cin >> std::ws, account);
        std::cout << "请输入你的密码:";
        password = cinkey();
        std::cout << std::endl;
        j["cmd"] = "signup";
        j["account"] = account;
        j["password"] = password;
        id = gen_req_id();
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_login_code(){
    if (is_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    }else{
        std::cout << "请先输入你的qq邮箱:";
        std::getline(std::cin >> std::ws, account);
        j["cmd"] = "verifycode";
        j["account"] = account;
        c_conn->send(j.dump() + '\n');
        std::cout << "验证码已经发送到您的qq邮箱\n";
        std::cout << "请输入验证码:";
        std::cin >> verifycode;
        j["cmd"] = "verifycodesignin";
        j["account"] = account;
        j["code"] = verifycode;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::string cc = res["code"];

        if (cc == "1") {
            is_login = true;
        }
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_login_key(){
    if (is_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    }else{
        std::cout <<PURPLE<< "请先输入你的qq邮箱:"<<RESET;
        std::getline(std::cin >> std::ws, account);
        std::cout <<PURPLE<< "请输入您的密码:"<<RESET;
        password = cinkey();
        j["cmd"] = "keysignin";
        j["account"] = account;
        j["password"] = password;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::string cc = res["code"];

        if (cc == "1") {
            is_login = true;
        }
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_forget_key(){
    if (is_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    }else{
        std::cout << "请输入你的qq邮箱: ";
        std::getline(std::cin >> std::ws, account);
        j["cmd"] = "forgetkey";
        j["account"] = account;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_destory(){
    if (is_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    }else{
        std::cout << PURPLE << "请先输入你的qq邮箱:" << RESET;
        std::getline(std::cin >> std::ws, account);
        std::cout << PURPLE << "请输入您的密码:" << RESET;
        password = cinkey();
        j["cmd"] = "destory";
        j["account"] = account;
        j["password"] = password;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_exit(){
    std::cout << "再见！" << std::endl;
    running = false;
    msg_cv.notify_all();
    event_cv.notify_all();
}
void Choicework::handle_addfriend(){
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    }else{
        std::cout <<PURPLE<< "请输入要添加好友的账号:"<<RESET;
        std::getline(std::cin >> std::ws, frienduser);
        j["cmd"] = "addfriend";
        j["from"] = account;
        j["to"] = frienduser;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_friendchat(){

}
void Choicework::handle_friendlist(){
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    }else{
        j["cmd"] = "friendlist";
        j["account"] = account;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_block(){
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    }else{
        std::cout << "请输入要拉黑的好友账号:";
        std::getline(std::cin >> std::ws, frienduser);
        j["cmd"] = "block";
        j["account"] = account;
        j["target"] = frienduser;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_disblock(){

}
void Choicework::handle_delfriend(){
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    }else{
        std::cout << "请输入要删除的好友账号:";
        std::getline(std::cin >> std::ws, frienduser);
        j["cmd"] = "delfriend";
        j["account"] = account;
        j["target"] = frienduser;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_creategroup(){
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    }else{
        std::cout << "请输入要创建的群聊的名字:";
        std::getline(std::cin >> std::ws, groupname);
        j["cmd"] = "creategroup";
        j["account"] = account;
        j["groupname"] = groupname;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_invite(){
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    }else{
        std::cout << "请输入要邀请好友加入的群聊名称:";
        std::getline(std::cin >> std::ws, groupname);
        std::cout << "请输入要邀请哪位好友加入该群聊:";
        std::getline(std::cin >> std::ws, frienduser);
        j["cmd"] = "invite";
        j["account"] = account;
        j["groupname"] = groupname;
        j["target"] = frienduser;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_exitgroup(){
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    }else{
        std::cout << "请输入要删除的群聊的名称:";
        std::getline(std::cin >> std::ws, groupname);
        j["cmd"] = "delgroup";
        j["account"] = account;
        j["groupname"] = groupname;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_addfriendmsg(){
    json reply;
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "好友申请消息：" << std::endl;
        for (auto i = 0; i < addlist.size();i++){
            std::cout <<GREEN<< addlist[i] <<RESET<< std::endl;
        }
        std::cout << "请选择要处理的账号:";
        std::getline(std::cin >> std::ws, frienduser);
        std::string ss;
        std::cout << std::endl;
        std::cout << "请输入y 同意|n 拒绝:";
        std::getline(std::cin >> std::ws, ss);
        if (ss == "y" || ss == "Y") {
            reply["cmd"] = "agreefriend";
            std::cout << "已同意好友申请" << std::endl;
        } else {
            reply["cmd"] = "refusefriend";
            std::cout << "已经拒绝好友申请" << std::endl;
        }
        reply["account"] = account;
        reply["friendaccount"] = frienduser;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_invitemsg(){
    json reply;
    if (!is_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::cout << "群聊邀请消息：" << std::endl;
        for (int i = 0; i < invitelist.size(); i++) {
            frienduser=invitelist[i]["account"];
            groupname = invitelist[i]["groupname"];
            std::cout <<GREEN<< "["<<i+1<<"]"<<frienduser << "邀请你进入群聊：" << groupname
                      << RESET<<std::endl;
        }
        std::cout << "请选择要处理的消息序号:";
        int choice;
        std::cin >> choice;
        for (int i = 0; i < invitelist.size();i++){
            if(i==choice-1){
                frienduser = invitelist[i]["account"];
                groupname = invitelist[i]["groupname"];
            }
        }
            std::string ss;
        std::cout << "请输入y 同意|n 拒绝:";
        std::getline(std::cin >> std::ws, ss);
        if (ss == "y" || ss == "Y") {
            reply["cmd"] = "agreejoin";
            std::cout << "已加入群聊:"<<groupname << std::endl;
        } else {
            reply["cmd"] = "refusejoin";
            std::cout << "已经拒绝加入群聊" <<groupname<< std::endl;
        }
        reply["account"] = account;
        reply["addaccount"] = frienduser;
        reply["groupname"] = groupname;
        j["requset_id"] = id;
        std::promise<json> p;
        std::future<json> f = p.get_future();
        {
            std::lock_guard<std::mutex> lock(active_mutex);
            active_requests[id] = std::move(p);
        }
        c_conn->send(j.dump() + '\n');
        json res = f.get();
        std::cout << res["data"] << std::endl;
    }
}
void Choicework::handle_groupchat() {}
