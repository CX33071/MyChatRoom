#include "ChatClient.h"
#include "FileClient.h"

chatclient::chatclient(EventLoop* loop, const InetAddress& addr):client_(loop,addr),loop_(loop){
    client_.setConnectionCallback(
        [this](const TcpClient::TcpConnectionPtr& conn) {
            connectioncallback(conn);
        });

    client_.setMessageCallback(
        [this](const TcpClient::TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
            messagecallback(conn, buf, t);
        });
    client_.connect();
    loop_->runEvery(30, [this]() { sendheart(); });
    tcgetattr(STDIN_FILENO, &oldt);
    rl_bind_keyseq("\\e[15~", emptyfunction);
    rl_bind_keyseq("\\e[17~", emptyfunction);
    rl_bind_keyseq("\\e[18~", emptyfunction);
    rl_bind_keyseq("\\e[19~", emptyfunction);
    rl_bind_keyseq("\\e[20~", emptyfunction);
    rl_bind_keyseq("\\e[24~", emptyfunction);
}
int chatclient::emptyfunction(int n1,int n2){
    return 0;
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
        if(!stop1){
            std::cout << PURPLE << "服务端断开连接" << std::endl;
        }
        // std::cout << "\nchatclientclose!\n";
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
int chatclient::changenum(std::string s){
    errno = 0;
    if (s.empty()) {
        return -1;
    }
    char*end;
    int num=strtol(s.c_str(),&end,10);
    if(errno==ERANGE){
        return -2;
    }
    if(*end!='\0'){
        return -1;
    }
    return num;
}
std::string chatclient::cinkey() {
    std::string password;
    termios newt;
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char* line = readline(PURPLE "请输入密码:" RESET);
    if (line == nullptr) {
        stop1 = true;
        if (chatis_login) {
            handle_exitlogin();
        }
        handle_exit();
        _exit(0);
    }
    password=line;
    free(line);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;
    return password;
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
    std::cout << GREEN << "0.               退出chatroom               \n" << RESET;
}
void chatclient::select_menu(){
    std::cout << GREEN << "1.            好友菜单               \n" << RESET;
    std::cout << GREEN << "2.            群聊菜单               \n" << RESET;
    std::cout << GREEN << "3.            文件菜单               \n" << RESET;
    std::cout << GREEN << "4.            消息菜单               \n" << RESET;
    std::cout << GREEN << "5.            退出登录\n";
    std::cout << GREEN << "0.               退出chatroom               \n"
              << RESET;
}
void chatclient::friend_menu() {
    std::cout << GREEN << "\n================好友菜单================\n"
              << RESET;
    std::cout << GREEN << "1.              添加好友               \n" << RESET;
    std::cout << GREEN << "2.              删除好友               \n" << RESET;
    std::cout << GREEN << "3.              拉黑好友               \n" << RESET;
    std::cout << GREEN << "4.              取消拉黑               \n" << RESET;
    std::cout << GREEN << "5.             好友列表               \n" << RESET;
    std::cout << GREEN << "6.           拉黑好友列表               \n"
              << RESET;
    std::cout << GREEN << "7.          处理好友申请消息               \n"
              << RESET;
    std::cout << GREEN << "8.              私聊               \n" << RESET;
    std::cout << GREEN << "9.          显示好友在线状态               \n"
              << RESET;
    std::cout << GREEN << "10.          返回功能菜单\n" << RESET;
    std::cout << GREEN << "11.          退出登录\n" << RESET;
    std::cout << GREEN << "0.          退出chatroom\n" << RESET<<std::flush;
}
void chatclient::group_menu(){
    std::cout << GREEN << "\n================群聊菜单================\n"
              << RESET;
    std::cout << GREEN << "1.             创建群聊               \n" << RESET;
    std::cout << GREEN << "2.           申请加入群聊               \n"
              << RESET;
    std::cout << GREEN << "3.             退出群聊               \n" << RESET;
    std::cout << GREEN << "4.            查看群聊成员               \n"
              << RESET;
    std::cout << GREEN << "5.           设置群聊管理员               \n"
              << RESET;
    std::cout << GREEN << "6.             解散群聊               \n" << RESET;
    std::cout << GREEN << "7.             群聊列表               \n" << RESET;
    std::cout << GREEN << "8.            移除群聊成员               \n"
              << RESET;
    std::cout << GREEN << "9.             群聊天               \n" << RESET;
    std::cout << GREEN << "10.           处理加群申请               \n"
              << RESET;
    std::cout << GREEN << "11.          返回功能菜单\n" << RESET;
    std::cout << GREEN << "12.          退出登录\n" << RESET;
    std::cout << GREEN << "0.          退出chatroom\n" << RESET;
}
void chatclient::msg_menu(){
    std::cout << GREEN
              << "\n================消息菜单================\n"<< RESET;
    std::cout << GREEN << "\n请选择要处理的消息类型编号:\n";
    std::cout << GREEN << "1.             好友申请消息               \n" << RESET;
    std::cout << GREEN << "2.           加群申请               \n" << RESET;
    std::cout << GREEN << "3.             传文件消息               \n" << RESET;
    std::cout << GREEN << "4.          返回功能菜单\n" << RESET;
    std::cout << GREEN << "5.          退出登录\n" << RESET;
    std::cout << GREEN << "0.          退出chatroom\n" << RESET;
}
void chatclient::file_menu(){
    std::cout << GREEN << "\n================文件功能================\n";
    std::cout << GREEN << "1.           好友上传文件               \n"
              << RESET;
    std::cout << GREEN << "2.           处理文件消息               \n"
              << RESET;
    std::cout << GREEN << "3.           群聊上传文件               \n"
              << RESET;
    std::cout << GREEN << "4.            继续上传文件            \n" << RESET;
    std::cout << GREEN << "5.            继续下载文件\n";
    std::cout << GREEN << "6.            返回功能菜单               \n"
              << RESET;
    std::cout << GREEN << "7.          退出登录\n" << RESET;
    std::cout << GREEN << "0.          退出chatroom               \n" << RESET;
}
bool chatclient::isQQemail(std::string email){
    std::regex pattern("^[1-9][0-9]{4,10}@qq\\.com$");
    return std::regex_match(email,pattern);
}
std::string chatclient::getaccount(std::string name){
    j["cmd"]="getaccount";
    j["name"] = name;
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
    return res["data"];
}
std::string chatclient::getname(std::string account){
    j["cmd"]="getname";
    j["account"]=account;
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
    return res["data"];
}
bool chatclient::is_existsname(std::string name){
    j["cmd"]="isexistsname";
    j["name"]=name;
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
    } else {
        return false;
    }
}
void chatclient::handle_signup() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        char* line = readline(PURPLE "请输入你的qq邮箱:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if(!is_email){
            std::cout << PURPLE << "QQ邮箱格式不正确" << RESET << std::endl;
            return;
        }
        bool b = is_exists(account);
        if(b){
            std::cout << PURPLE << "该账号已经存在，请登录!" << RESET
                      << std::endl;
            return;
        }
        password = cinkey();
        if (password.empty()) {
            std::cout << PURPLE << "密码不能为空" << RESET << std::endl;
            return;
        }
        std::cout << std::endl;
        line = readline(PURPLE "请输入你的昵称:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        name= line;
        free(line);
        while (is_existsname(name)) {
            line =
                readline(PURPLE "该昵称已被用户使用，请重新输入昵称:" RESET);
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            name = line;
            free(line);
        }
        j["cmd"] = "signup";
        j["account"] = account;
        j["password"] = password;
        j["name"] = name;
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
bool chatclient::is_online(std::string account){
    j["cmd"]="is_online";
    j["account"]=account;
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
    }else{
        return false;
    }
}
void chatclient::handle_login_code() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        char* line = readline(PURPLE "请先输入你的qq邮箱:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if (!is_email) {
            std::cout << PURPLE << "QQ邮箱格式不正确" << RESET << std::endl;
            return;
        }
        bool b=is_exists(account);
        if(!b){
            std::cout << PURPLE << "该帐号并不存在，请先注册!" << RESET
                      << std::endl;
            return;
        }else{
            if(is_online(account)){
                std::cout << PURPLE << "该账号正处于登录状态!" << RESET
                          << std::endl;
                return;
            }
        }
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
        std::cout << "验证码已经发送到您的qq邮箱\n" << std::flush;
        line = readline(PURPLE "请输入验证码:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        verifycode = line;
        free(line);
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
            name = getname(account);
            system("clear");
            SQ.open(name);
        } else {
            std::cout << res["data"] << std::endl;
        }
    }
}
void chatclient::handle_login_key() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        char* line = readline(PURPLE "请先输入你的qq邮箱:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if (!is_email) {
            std::cout << PURPLE << "QQ邮箱格式不正确" << RESET << std::endl;
            return;
        }
        if(!is_exists(account)){
            std::cout << PURPLE << "该账号并不存在，请先注册!" << RESET
                      << std::endl;
            return;
        } else {
            if (is_online(account)) {
                std::cout << PURPLE << "该账号正处于登录状态!" << RESET
                          << std::endl;
                return;
            }
        }
        password = cinkey();
        if (password.empty()) {
            std::cout << PURPLE << "密码不能为空" << RESET << std::endl;
            return;
        }
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
            name = getname(account);
            system("clear");
            SQ.open(name);
            handle_uploadcheck();
            handle_downloadcheck();
        } else {
            std::cout << res["data"] << std::endl;
        }
    }
}
void chatclient::handle_uploadcheck(){
    j["cmd"]="uploadcheck";
    j["account"]=account;
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
    std::vector<json> data=res.value("data",std::vector<json>{});
    if(!data.empty()){
        std::cout << PURPLE << "你有" << data.size() << "个文件上传失败"
                  << std::endl;
        for (int i = 0; i < data.size();i++){
            double cur = std::stoll(data[i]["sended"].get<std::string>()) / 1000000.0;
            double total = std::stoll(data[i]["total"].get<std::string>()) / 1000000.0;
            std::cout <<std::fixed<<std::setprecision(1) << "[" << i + 1 << "] " << data[i]["filename"]
                      << "    上传" << cur << "/"
                      << total<<" MB" << std::endl;
            filestatus f;
            f.sended=data[i]["sended"];
            f.total=data[i]["total"];
            f.filename=data[i]["filename"];
            f.id=data[i]["fileid"];
            uploads.push_back(f);
        }
        std::cout << "请稍候在文件菜单中查看详细" << RESET<<std::endl;
    }
}
void chatclient::handle_downingfile(){
   if(downloads.empty()){
       std::cout << PURPLE << "当前并没有下载失败文件" << RESET << std::endl;
       return;
   }
   for (int i = 0; i < downloads.size();i++){
       std::cout << PURPLE << "[" << i + 1 << "]    来自"<<downloads[i].from<<" " << downloads[i].filename
                 << "  下载" << downloads[i].recived << "/"
                 << downloads[i].total << std::endl;
   }
   char *line=readline(PURPLE"请选择你要处理的编号:"RESET);
   int num;
   if (line == nullptr) {
       stop1 = true;
       if (chatis_login) {
           handle_exitlogin();
       }
       handle_exit();
       _exit(0);
   }
   std::string reads = line;
   free(line);
   num = changenum(reads);
   while (num == -1) {
       line = readline(PURPLE "非法输入，请重新输入:" RESET);
       if (line == nullptr) {
           stop1 = true;
           if (chatis_login) {
               handle_exitlogin();
           }
           handle_exit();
           _exit(0);
       }
       reads = line;
       free(line);
       num = changenum(reads);
   }
   int total = downloads.size();
   while (num > total || num < 1) {
       line = readline(PURPLE "请输入要继续上传的文件消息编号!请选择:" RESET);
       if (line == nullptr) {
           stop1 = true;
           if (chatis_login) {
               handle_exitlogin();
           }
           handle_exit();
           _exit(0);
       }
       reads = line;
       free(line);
       num = changenum(reads);
       while (num == -1) {
           line = readline(PURPLE "非法输入，请重新输入:" RESET);
           if (line == nullptr) {
               stop1 = true;
               if (chatis_login) {
                   handle_exitlogin();
               }
               handle_exit();
               _exit(0);
           }
           reads = line;
           free(line);
           num = changenum(reads);
       }
   }
   std::string fileid = downloads[num - 1].id;
   filename = downloads[num - 1].filename;
   std::string from = downloads[num - 1].from;
   std::string filesize = downloads[num - 1].total;
   j["cmd"]="redownloadfile";
   j["fileid"]=fileid;
   j["from"] = from;
   j["filename"] = filename;
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
   std::string downfilepath = res["filepath"];
   std::string downsize = res["downsize"];
   int n = fileclient_->reloadfile(filename, filesize, fileid, downfilepath,downsize);
   if(n!=-1){
       downloads.erase(downloads.begin() + num - 1);
   }
}
void chatclient::handle_downloadcheck(){
    j["cmd"] = "downloadcheck";
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
    std::vector<json> data = res.value("data", std::vector<json>{});
    if (!data.empty()) {
        std::cout << PURPLE << "你有" << data.size() << "个文件下载失败"
                  << std::endl;
        for (int i = 0; i < data.size(); i++) {
            double cur =
                std::stoll(data[i]["recived"].get<std::string>()) / 1000000.0;
            double total =
                std::stoll(data[i]["total"].get<std::string>()) / 1000000.0;
            std::cout << std::fixed << std::setprecision(1) << "[" << i + 1
                      << "]     来自"<<data[i]["from"] <<"  "<< data[i]["filename"] << "  下载" << cur << "/"
                      << total << " MB" << std::endl;
            filestatus f;
            f.recived = data[i]["recived"];
            f.total = data[i]["total"];
            f.filename = data[i]["filename"];
            f.id = data[i]["fileid"];
            f.from = data[i]["from"];
            downloads.push_back(f);
        }
        std::cout << "请稍候再文件菜单中查看详细" << RESET << std::endl;
    }
}
void chatclient::handle_forget_key() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        char* line = readline(PURPLE "请先输入你的qq邮箱:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if (!is_email) {
            std::cout << PURPLE << "QQ邮箱格式不正确" << RESET << std::endl;
            return;
        }
        if (!is_exists(account)) {
            std::cout << PURPLE << "该账号并不存在，请先注册!" << RESET
                      << std::endl;
            return;
        }
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
        char* line = readline(PURPLE "请先输入你的qq邮箱:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if (!is_email) {
            std::cout << PURPLE << "QQ邮箱格式不正确" << RESET << std::endl;
            return;
        }
        if (!is_exists(account)) {
            std::cout << PURPLE << "该账号并不存在，请先注册!" << RESET
                      << std::endl;
            return;
        }
        line = readline(PURPLE "请输入您的密码:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        password = line;
        free(line);
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
    std::cout << std::endl;
    std::cout<<PURPLE << "再见！" <<RESET<< std::endl;
    running = false;
    msg_cv.notify_all();
    event_cv.notify_all();
    chatis_login = false;
    if(chat_conn){
        chat_conn->forceClose();
    }
    if(fileclient_->file_conn){
        fileclient_->file_conn->forceClose();
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    loop_->quit();
}
void chatclient::handle_addfriend() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        char* line = readline(PURPLE "请输入要添加好友的账号:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        frienduser = line;
        free(line);
        bool is_email = isQQemail(frienduser);
        if (!is_email) {
            std::cout << PURPLE << "QQ邮箱格式不正确" << RESET << std::endl;
            return;
        }
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
        char* line = readline(PURPLE "请输入你要加入群聊的名称:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        groupname = line;
        free(line);
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
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
        chatis_login = false;
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
    if (res["code"] == "1") {
        return 1;
    } else if (res["code"] == "2") {
        return 2;
    } else {
        return 0;
    }
}
void chatclient::handle_getfriendchat(std::string friendaccount){
    j["cmd"] = "getfriendchathistory";
    j["account"]=account;
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
    std::vector<json> data = res.value("data", std::vector<json>{});
    if (data.size() != 0) {
        for (auto t : data) {
            std::string sender = t["sender"];
            std::string reciver = t["reciver"];
            std::string content = t["content"];
            std::string s1 = getname(sender);
            std::string s2 = getname(reciver);
            SQ.addfriendchat(s1, s2, content);
        }
    }
}
void chatclient::handle_friendchat() {
    char* line = readline(PURPLE "请输入要私聊的好友名称:" RESET);
    if (line == nullptr) {
        stop1 = true;
        if (chatis_login) {
            handle_exitlogin();
        }
        handle_exit();
        _exit(0);
    }
    std::string friendname = line;
    free(line);
    if (!is_existsname(friendname)) {
        std::cout << PURPLE << "该用户并不存在!" << RESET << std::endl;
        return;
    }
    current_chat = getaccount(friendname);
    friendhistory.clear();
    int is = is_friend(current_chat);
    if (is == 1) {
        std::cout << "该账号并不存在" << std::endl;
    } else if (is == 0) {
        std::cout << "对方目前还不是您的好友" << std::endl;
    } else {
        int N;
        line = readline(
            PURPLE "请选择是否要查看所有历史聊天记录1/开启新聊天2:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        std::string reads = line;
        free(line);
        N=changenum(reads);
        while(N!=1&&N!=2){
            if(N==-1){
                line = readline(PURPLE "非法输入，请重新输入:" RESET);
            }else{
                line = readline(PURPLE "请输入数字1-2:" RESET);
            }
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            std::string reads = line;
            free(line);
            N = changenum(reads);
        }
        inchat = true;
        system("clear");
        std::cout << GREEN << "进入与用户[" << friendname << "]的聊天界面"
                  << RESET << std::endl;
        std::cout << GREEN << "输入 EXIT 退出当前聊天" << RESET << std::endl;
        if(N==1){
            std::cout << GREEN << "所有聊天记录:" << RESET << std::endl;
            std::vector<std::string> chat = SQ.getfriendchat(name, friendname);
            if (chat.empty()) {
                handle_getfriendchat(current_chat);
                chat = SQ.getfriendchat(name, friendname);
            }
            for (int i = 0; i < chat.size(); i++) {
                std::cout << chat[i] << std::endl;
            }
        } else {
            std::cout << GREEN << "最近十条历史聊天:" << RESET << std::endl;
            std::vector<std::string> chat = SQ.getfriendchat(name, friendname);
            if (chat.empty()) {
                handle_getfriendchat(current_chat);
                chat = SQ.getfriendchat(name, friendname);
            }
            if (chat.size() <= 10) {
                for (int i = 0; i < chat.size(); i++) {
                    std::cout << chat[i] << std::endl;
                }
            } else {
                int total = 0;
                for (int i = chat.size() - 10; total < 10; i++) {
                    std::cout << chat[i] << std::endl;
                    total++;
                }
            }
        }
        std::cout << GREEN << "开启新的聊天!" << RESET << std::endl;
        while (inchat) {
            std::string msg;
            std::string s = PURPLE;
            s += "[";
            s += name;
            s += "]:   ";
            s+=RESET;
            line = readline(s.c_str());
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            msg = line;
            free(line);
            if (msg == "EXIT") {
                inchat = false;
                break;
            }
            SQ.addfriendchat(name, friendname, msg);
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
        std::cout << std::endl;
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
        char* line = readline(PURPLE "请输入要管理的群聊名称:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        groupname = line;
        free(line);
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
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
        int num;
        line =
            readline(PURPLE "请输入要添加管理员1 / 删除管理员2:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        std::string reads = line;
        free(line);
        num = changenum(reads);
        while (num != 1 && num != 2) {
            if (num == -1) {
                line = readline(PURPLE "非法输入，请重新输入:" RESET);
            } else {
                line = readline(PURPLE "请输入数字1-2:" RESET);
            }
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            reads = line;
            free(line);
            num = changenum(reads);
        }
        if (num == 1) {
            std::string count;
            std::string friendname;
            line = readline(PURPLE "请输入要添加为管理员的用户:" RESET);
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            friendname = line;
            free(line);
            b = is_existsname(friendname);
            if(!b){
                std::cout << PURPLE << "该用户并不存在" << RESET << std::endl;
                return;
            }
            count = getaccount(friendname);
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
            std::string friendname;
            line = readline(PURPLE "请输入要删除的管理员:" RESET);
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            friendname = line;
            free(line);
            if (!is_existsname(friendname)) {
                std::cout << PURPLE << "该用户并不存在!" << RESET << std::endl;
                return;
            }
            count = getaccount(friendname);
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
        char* line = readline(PURPLE "请输入要查看群成员的群聊名称:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        groupname = line;
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
        free(line);
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
        std::string friendname;
        char* line = readline(PURPLE "请输入要拉黑的好友:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        friendname = line;
        free(line);
        if (!is_existsname(friendname)) {
            std::cout << PURPLE << "该用户并不存在" << RESET << std::endl;
            return;
        }
        frienduser = getaccount(friendname);
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
    if(res["code"]=="1"){
        return true;
    }else{
        return false;
    }
}
void chatclient::handle_ownergrouplist(){
    if(!chatis_login){
        std::cout << "请先登录!" << std::endl;
    }else{
        j["cmd"]="ownergrouplist";
        j["account"]=account;
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
void chatclient::handle_disblock() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        std::string friendname;
        char* line = readline(PURPLE "请输入要取消拉黑的好友:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        friendname = line;
        free(line);
        if (!is_existsname(friendname)) {
            std::cout << PURPLE << "该用户并不存在" << RESET << std::endl;
            return;
        }
        frienduser = getaccount(friendname);
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
        std::string friendname;
        char* line = readline(PURPLE "请输入要删除的好友:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        friendname = line;
        free(line);
        if (!is_existsname(friendname)) {
            std::cout << PURPLE << "该用户并不存在" << RESET << std::endl;
            return;
        }
        frienduser = getaccount(friendname);
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
        char* line = readline(PURPLE "请输入要创建的群聊的名字:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        groupname = line;
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
        free(line);
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
        char* line = readline(PURPLE "请输入要退出的群聊的名称:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        groupname = line;
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
        free(line);
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
    char* line = readline(PURPLE "请输入您要删除成员的群聊名称:" RESET);
    if (line == nullptr) {
        stop1 = true;
        if (chatis_login) {
            handle_exitlogin();
        }
        handle_exit();
        _exit(0);
    }
    groupname = line;
    if (groupname.empty()) {
        std::cout << PURPLE << "群名不能为空" << std::endl;
        return;
    }
    free(line);
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
    std::string friendname;
    std::string count;
    line = readline(PURPLE "请输入您要删除的成员:" RESET);
    if (line == nullptr) {
        stop1 = true;
        if (chatis_login) {
            handle_exitlogin();
        }
        handle_exit();
        _exit(0);
    }
    friendname = line;
    free(line);
    if (!is_existsname(friendname)) {
        std::cout << PURPLE << "该用户并不存在" << RESET << std::endl;
        return;
    }
    count = getaccount(friendname);
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
        char* line = readline(PURPLE "请输入要解散的群聊名称:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        groupname = line;
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
        free(line);
        j["cmd"] = "delgroup";
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
            std::cout <<PURPLE<< "好友申请消息：" << std::endl;
            for (auto i = 0; i < addlist.size(); i++) {
                std::string friendname = getname(addlist[i]["account"]);
                std::cout << GREEN << "[" << i + 1
                          << "]:" << friendname
                          << "      消息时间:" << addlist[i]["time"] << RESET
                          << std::endl;
            }
            int num;
            char* line = readline(PURPLE "请选择要处理的消息编号:" RESET);
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            std::string reads = line;
            free(line);
            num = changenum(reads);
            while(num==-1){
                line = readline(PURPLE "非法输入，请重新输入:" RESET);
                if (line == nullptr) {
                    stop1 = true;
                    if (chatis_login) {
                        handle_exitlogin();
                    }
                    handle_exit();
                    _exit(0);
                }
                reads = line;
                free(line);
                num = changenum(reads);
            }
            int total = addlist.size();
            while(num>total||num<1){
                line = readline(PURPLE "请输入正确消息编号!请选择:" RESET);
                if (line == nullptr) {
                    stop1 = true;
                    if (chatis_login) {
                        handle_exitlogin();
                    }
                    handle_exit();
                    _exit(0);
                }
                reads = line;
                free(line);
                num=changenum(reads);
                while (num == -1) {
                    line = readline(PURPLE "非法输入，请重新输入:" RESET);
                    if (line == nullptr) {
                        stop1 = true;
                        if (chatis_login) {
                            handle_exitlogin();
                        }
                        handle_exit();_exit(0);
                    }
                    reads = line;
                    free(line);
                    num = changenum(reads);
                }
            }
            frienduser = addlist[num - 1]["account"];
            std::string ss;
            std::cout << std::endl;
            line = readline(PURPLE "请输入y 同意|n 拒绝:" RESET);
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            ss = line;
            free(line);
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
                    line = readline(PURPLE "请输入y 同意|n 拒绝:" RESET);
                    if (line == nullptr) {
                        stop1 = true;
                        if (chatis_login) {
                            handle_exitlogin();
                        }
                        handle_exit();
                        _exit(0);
                    }
                    ss = line;
                    free(line);
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
std::vector<std::string> chatclient::getlocalfile(std::string path){
    std::vector<std::string> list;
    DIR*dir=opendir(path.c_str());
    if(dir==nullptr){
        perror("opendir");
        return list;
    }
    struct dirent* entry;
    while((entry=readdir(dir))!=nullptr){
        std::string name = entry->d_name;
        if (name == "."||name==".."){
            continue;
        }
        list.push_back(name);
    }
    closedir(dir);
    return list;
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
           int num;
           char* line = readline(PURPLE "请选择要处理的消息编号:" RESET);
           if (line == nullptr) {
               stop1 = true;
               if (chatis_login) {
                   handle_exitlogin();
               }
               handle_exit();
               _exit(0);
           }
           std::string reads = line;
           free(line);
           num = changenum(reads);
           while (num == -1) {
                line = readline(PURPLE "非法输入，请重新输入:" RESET);
                if (line == nullptr) {
                    stop1 = true;
                    if (chatis_login) {
                        handle_exitlogin();
                    }
                    handle_exit();
                    _exit(0);
                }
               reads = line;
               free(line);
               num = changenum(reads);
           }
           while (num > sendfilelist.size()||num<1) {
                line = readline(PURPLE "请输入正确消息编号!请选择:" RESET);
                if (line == nullptr) {
                    stop1 = true;
                    if (chatis_login) {
                        handle_exitlogin();
                    }
                    handle_exit();_exit(0);
                }
               reads = line;
               free(line);
               num = changenum(reads);
               while (num == -1) {
                    line = readline(PURPLE "非法输入，请重新输入:" RESET);
                    if (line == nullptr) {
                        stop1 = true;
                        if (chatis_login) {
                            handle_exitlogin();
                        }
                        handle_exit();
                        _exit(0);
                    }
                   reads = line;
                   free(line);
                   num = changenum(reads);
               }
           }
             line =
               readline(PURPLE "请输入您要下载文件到本地的路径：" RESET);
             if (line == nullptr) {
                 stop1 = true;
                 if (chatis_login) {
                     handle_exitlogin();
                 }
                 handle_exit();
                 _exit(0);
             }
           filepath = line;
           free(line);
           std::cout << RESET << std::endl;
           std::string from = sendfilelist[num - 1]["from"];
           filename = sendfilelist[num - 1]["filename"];
           std::vector<std::string> pathfile = getlocalfile(filepath);
           for(auto t:pathfile){
            if(t==filename){
                std::cout << PURPLE << "你的本地路径" << filepath
                          << "里已经存在文件" << t << ",要替换吗？" 
                          << std::endl;
                std::string ss;
                 line = readline(PURPLE "请输入替换y|Y,放弃n|N:" RESET);
                 if (line == nullptr) {
                     stop1 = true;
                     if (chatis_login) {
                         handle_exitlogin();
                     }
                     handle_exit();
                     _exit(0);
                 }
                ss = line;
                free(line);
                while(true){
                    if (ss == "y" || ss == "Y") {
                        break;
                    } else if (ss == "n" || ss == "N") {
                        return;
                    } else {
                          line =
                            readline(PURPLE "请输入替换y|Y,放弃n|N:" RESET);
                          if (line == nullptr) {
                              stop1 = true;
                              if (chatis_login) {
                                  handle_exitlogin();
                              }
                              handle_exit();
                              _exit(0);
                          }
                        ss = line;
                        free(line);
                    }
                }
               
            }
           }
           reply["cmd"] = "recvfile";
           reply["from"] = from;
           reply["filepath"]=filepath;
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
           json res = f.get();
           std::string filesize = res["filesize"];
           int n=fileclient_->loadfile(filename, filesize, ID, filepath);
           if(n!=-1){
               sendfilelist.erase(sendfilelist.begin() + num - 1);
           }
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
           std::cout<<PURPLE << "申请加入群聊消息：" << std::endl;
           for (auto i = 0; i < applyjoinlist.size(); i++) {
               std::cout << GREEN << "[" << i + 1 << "]"
                         << applyjoinlist[i]["data"] << RESET << std::endl;
           }
           int num;
           char* line = readline(PURPLE "请选择要处理的消息编号:" RESET);
           if (line == nullptr) {
               stop1 = true;
               if (chatis_login) {
                   handle_exitlogin();
               }
               handle_exit();
               _exit(0);
           }
          std::string reads = line;
           free(line);
           num = changenum(reads);
           while (num == -1) {
                 line = readline(PURPLE "非法输入，请重新输入:" RESET);
                 if (line == nullptr) {
                     stop1 = true;
                     if (chatis_login) {
                         handle_exitlogin();
                     }
                     handle_exit();
                     _exit(0);
                 }
               reads = line;
               free(line);
               num = changenum(reads);
           }
           while (num > applyjoinlist.size()||num<1) {
                line = readline(PURPLE "请输入正确的编号!请选择:" RESET);
                if (line == nullptr) {
                    stop1 = true;
                    if (chatis_login) {
                        handle_exitlogin();
                    }
                    handle_exit();
                    _exit(0);
                }
               reads = line;
               free(line);
               num = changenum(reads);
               while (num == -1) {
                    line = readline(PURPLE "非法输入，请重新输入:" RESET);
                    if (line == nullptr) {
                        stop1 = true;
                        if (chatis_login) {
                            handle_exitlogin();
                        }
                        handle_exit();
                        _exit(0);
                    }
                   reads = line;
                   free(line);
                   num = changenum(reads);
               }
           }
           std::cout << std::endl;
           std::string g = applyjoinlist[num - 1]["groupname"];
           std::string a = getname(applyjoinlist[num - 1]["from"]);
           std::cout<<PURPLE << "请处理来自用户:" << a << "加入群聊" << g
                     << "的消息:" << std::endl;
           std::string ss;
            line = readline(PURPLE "请输入y 同意|n 拒绝:" RESET);
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();_exit(0);
            }
           ss = line;
           free(line);
           std::string a1 = applyjoinlist[num - 1]["from"];
           applyjoinlist.erase(applyjoinlist.begin() + num - 1);
           while (true) {
               if (ss == "y" || ss == "Y") {
                   reply["cmd"] = "agreejoingroup";
                   break;
               } else if (ss == "n" || ss == "N") {
                   reply["cmd"] = "refusejoingroup";
                   break;
               } else {
                    line = readline(PURPLE "请输入y 同意|n 拒绝:" RESET);
                    if (line == nullptr) {
                        stop1 = true;
                        if (chatis_login) {
                            handle_exitlogin();
                        }
                        handle_exit();
                        _exit(0);
                    }
                   ss = line;
                   free(line);
               }
           }
           reply["account"] = a1;
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
void chatclient::handle_uploadingfile(){
    if(uploads.empty()){
        std::cout << PURPLE << "当前并没有上传失败文件" << RESET << std::endl;
        return;
    }
    for (int i = 0; i < uploads.size();i++){
        std::cout<<PURPLE << "[" << i + 1 << "]     " << uploads[i].filename
                  << "   上传" << uploads[i].sended << "/" << uploads[i].total
                  << std::endl;
    }
    char *line=readline(PURPLE"请选择你要处理的编号:"RESET);
    int num;
    if (line == nullptr) {
        stop1 = true;
        if (chatis_login) {
            handle_exitlogin();
        }
        handle_exit();
        _exit(0);
    }
    std::string reads = line;
    free(line);
    num = changenum(reads);
    while (num == -1) {
        line = readline(PURPLE "非法输入，请重新输入:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        reads = line;
        free(line);
        num = changenum(reads);
    }
    int total=uploads.size();
    while (num > total || num < 1) {
        line = readline(PURPLE "请输入要继续上传的文件消息编号!请选择:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        reads = line;
        free(line);
        num = changenum(reads);
        while (num == -1) {
            line = readline(PURPLE "非法输入，请重新输入:" RESET);
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            reads = line;
            free(line);
            num = changenum(reads);
        }
    }
    std::string fileid = uploads[num - 1].id;
    filename = uploads[num - 1].filename;
    ;
    std::string filesize = uploads[num - 1].total;
    j["cmd"] = "resendfile";
    j["fileid"]=fileid;
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
    bool b;
    std::string to = res["to"];
    if (!is_exists(to)){
        b = true;
    }else{
        b = false;
    }
        std::string uploaded = res["uploaded"];
    std::string filepath = res["filepath"];
    fileclient_->uploadedsendfile(fileid, uploaded, filepath, filename,
                                  filesize,b);
    uploads.erase(uploads.begin() + num - 1);
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
        std::vector<json> data = res.value("data", std::vector<json>{});
        if(data.size()!=0){
            for(auto t:data){
                std::string sender = t["sender"];
                std::string content=t["content"];
                std::string n1 = getname(sender);
                SQ.addgroupchat(groupname, n1, content);
            }
        }
    }
}
void chatclient::handle_groupchat() {
    if (!chatis_login) {
        std::cout << "请先登录!" << std::endl;
    } else {
        char* line = readline(PURPLE "请输入要进行群聊的群聊名称:" RESET);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        groupname = line;
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
        free(line);
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
        int N;
         line = readline(
            PURPLE "请选择是否要查看所有历史聊天记录1/开启新聊天2:" RESET);
         if (line == nullptr) {
             stop1 = true;
             if (chatis_login) {
                 handle_exitlogin();
             }
             handle_exit();
             _exit(0);
         }
        std::string reads = line;
        free(line);
        N = changenum(reads);
        while(N!=1&&N!=2){
            if(N==-1){
                 line = readline(PURPLE "非法输入，请重新输入:" RESET);
            }else{
                 line = readline(PURPLE "请输入数字1-2:" RESET);
            }
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                _exit(0);
            }
            reads = line;
            free(line);
            N = changenum(reads);
        }
        groupchat = true;
        system("clear");
        std::cout << GREEN << "欢迎进入群聊[" << groupname << "]的聊天界面"
                  << RESET << std::endl;
        std::cout << GREEN << "输入 EXIT 退出当前聊天" << RESET << std::endl;
        if(N==1){
            std::cout << GREEN << "所有聊天记录:" << RESET << std::endl;
            std::vector<std::string> chat=SQ.getgroupchat(groupname);
            if(chat.empty()){
                getgrouphistory(groupname);
                chat = SQ.getgroupchat(groupname);
            }
            for (int i = 0; i < chat.size();i++){
                std::cout << chat[i] << std::endl;
            }
        }else{
            std::cout << GREEN << "最近十条历史聊天:" << RESET << std::endl;
            std::vector<std::string> chat = SQ.getgroupchat(groupname);
            if (chat.empty()) {
                getgrouphistory(groupname);
                chat = SQ.getgroupchat(groupname);
        }
        if(chat.size()<=10){
            for(int i=0;i<chat.size();i++){
                std::cout << chat[i] << std::endl;
            }
        }
        else{
            int total=0;
            for(int i=chat.size()-10;total<10;i++){
                std::cout << chat[i] << std::endl;
                total++;
            }
        }
    }
    std::cout << GREEN << "开启新聊天!" << RESET << std::endl;
    while (groupchat) {
        std::string msg;
        std::string s = GREEN;
        s += "[" + name + "]" + RESET+":";
         line = readline(s.c_str());
         if (line == nullptr) {
             stop1 = true;
             if (chatis_login) {
                 handle_exitlogin();
             }
             handle_exit();
             _exit(0);
         }
        msg = line;
        free(line);
        if (msg == "EXIT") {
            groupchat = false;
            break;
        }
        SQ.addgroupchat(groupname, name, msg);
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
        handle_friendlist();
        std::string friendname;
        char* line = readline(PURPLE"请输入您要发送文件的用户:"GREEN);
        if (line == nullptr) {
            stop1 = true;
            if (chatis_login) {
                handle_exitlogin();
            }
            handle_exit();
            _exit(0);
        }
        friendname= line;
        free(line);
        bool b = is_existsname(friendname);
        if (!b) {
            std::cout <<PURPLE<< "该用户并不是您的好友" << RESET << std::endl;
            return;
        }
        frienduser = getaccount(friendname);
        int b1 = is_friend(frienduser);
        if (b1 == 1) {
            std::cout << "该用户账号不存在" << std::endl;
            return;
        }
        if (b1 == 0) {
            std::cout << "该用户目前还不是您的好友，不能传文件" << std::endl;
            return;
        }
         line = readline(PURPLE "请输入您要上传的文件路径:" GREEN);
         if (line == nullptr) {
             stop1 = true;
             if (chatis_login) {
                 handle_exitlogin();
             }
             handle_exit();
             _exit(0);
         }
        filepath= line;
        free(line);
        if(filepath.empty()){
            std::cout << PURPLE << "上传文件路径为空，文件打开失败!" << RESET
                      << std::endl;
            return;
        }
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
        char buf[PATH_MAX];
        strcpy(buf, filepath.c_str());
        filename = basename(buf);
        json j;
        j["cmd"] = "sendfile";
        j["from"] = account;
        j["to"] = frienduser;
        j["filename"] = filename;
        j["filepath"] = filepath;
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
    handle_grouplist();
    std::string groupname;
    char* line = readline(PURPLE "请输入您要发送文件群聊名称:" GREEN);
    if (line == nullptr) {
        stop1 = true;
        if (chatis_login) {
            handle_exitlogin();
        }
        handle_exit();
        _exit(0);
    }
    groupname= line;
    free(line);
    bool b= is_existsgroup(groupname);
    if(!b){
        std::cout << PURPLE << "该群聊并不存在" << RESET << std::endl;
        return;
    }else if(!is_groupmember(groupname,account)){
        std::cout << PURPLE << "你并不在该群聊里" << RESET << std::endl;
        return;
    }else{
         line = readline(PURPLE "请输入你要上传文件的路径:" GREEN);
         if (line == nullptr) {
             stop1 = true;
             if (chatis_login) {
                 handle_exitlogin();
             }
             handle_exit();
             _exit(0);
         }
        filepath= line;
        free(line);
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
        j["filepath"] = filepath;
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