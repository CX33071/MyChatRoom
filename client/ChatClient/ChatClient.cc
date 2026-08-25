#include "ChatClient.h"
#include "FileClient/FileClient.h"
#define BG_SELECT "\033[48;5;238m"
#define WHITE "\033[37m"
bool cancelinput = false;
chatclient* chatclient::chatptr = nullptr;
int cancel_function(int n1,int n2) {
    rl_replace_line("",0);
    rl_crlf();
    rl_done = 1;
    cancelinput = true;
    return 0;
}
chatclient::chatclient(EventLoop* loop, const InetAddress& addr):client_(loop,addr),loop_(loop){
    chatptr = this;
    client_.setConnectionCallback(
        [this](const TcpClient::TcpConnectionPtr& conn) {
            connectioncallback(conn);
        });

    client_.setMessageCallback(
        [this](const TcpClient::TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
            messagecallback(conn, buf, t);
        });
    client_.connect();
    loop_->runEvery(1, [this]() { sendheart(); });
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ICANON | ECHO | ISIG;
    term.c_iflag |= ICRNL;
    term.c_oflag |= OPOST;
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    oldt = term;
    rl_bind_keyseq("\\e[15~", emptyfunction);
    rl_bind_keyseq("\\e[17~", emptyfunction);
    rl_bind_keyseq("\\e[18~", emptyfunction);
    rl_bind_keyseq("\\e[19~", emptyfunction);
    rl_bind_keyseq("\\e[20~", emptyfunction);
    rl_bind_keyseq("\\e[24~", emptyfunction);
    rl_bind_key('\007', cancel_function);
    rl_bind_key(0x1f,exechatmenu );
}
bool chatclient::inputvail(char*line){
    if (line == nullptr) {
        stop1 = true;
        if (chatis_login) {
            handle_exitlogin();
        }
        handle_exit();
        return false;
    }
    if (line[0] == '\0') {
        std::cout << "取消选择" << std::endl;
        free(line);
        return false;
    }
    return true;
}
void chatclient::cancel_requests() {
    std::lock_guard<std::mutex> lock(active_mutex);
    for (auto& [id, p] : active_requests) {
        json j;
        j["cmd"] = "cancel";
        p.set_value(j);
    }
    active_requests.clear();
}
int chatclient::emptyfunction(int n1,int n2){
    return 0;
}
int chatclient::exechatmenu(int n1,int n2){
    chatptr->chat_menu_index = -1;
    rl_done = 1;
    rl_replace_line("", 0);
    rl_on_new_line();
    tcflush(STDIN_FILENO, TCIFLUSH);
    int index = 0;
    chatptr->showchatmenu(0);
    char p;
    while (chatptr->running) {
        read(STDIN_FILENO, &p, 1);
        char p1, p2;
        if (p == '\033') {
            read(STDIN_FILENO, &p1, 1);
            read(STDIN_FILENO, &p2, 1);
            if (p1 == '[' && p2 == 'A') {
                chatptr->clearchatmenu();
                index--;
                if (index < 0) {
                    index = 8;
                }
                chatptr->showchatmenu(index);
            }
            if (p1 == '[' && p2 == 'B') {
                chatptr->clearchatmenu();
                index++;
                if (index > 8) {
                    index = 0;
                }
                chatptr->showchatmenu(index);
            }
        }
        if (p == '\r'||p=='\n') {
            chatptr->clearchatmenu();
            chatptr->chat_menu_index = index;
            rl_done = 1;
            break;
        }
    }
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
        if(running){
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
    if (!inputvail(line)) {
        return "";
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
    std::cout << GREEN << "1.            好友菜单               \n" ;
    std::cout << "2.            群聊菜单               \n" ;
    std::cout << "3.            文件菜单               \n" ;
    std::cout  << "4.            消息菜单               \n" ;
    std::cout << GREEN << "5.            修改昵称\n";
    std::cout << GREEN << "6.            退出登录\n";
    std::cout  << "0.               退出chatroom            \n"<< RESET;
}
void chatclient::friend_menu() {
std::cout << GREEN << "\n================好友菜单================\n";
    std::cout << "1.              添加好友               \n" ;
    std::cout << "2.              删除好友               \n" ;
    std::cout  << "3.              拉黑好友               \n" ;
    std::cout << "4.              取消拉黑               \n";
    std::cout  << "5.             好友列表               \n";
    std::cout  << "6.           拉黑好友列表               \n";
    std::cout << "7.          处理好友申请消息               \n";
    std::cout << "8.              私聊               \n" ;
    std::cout << "9.          显示好友在线状态               \n";
    std::cout  << "10.          返回功能菜单\n" ;
    std::cout  << "11.          退出登录\n" ;
    std::cout << "0.          退出chatroom\n" << RESET<<std::flush;
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
    std::cout << GREEN << "11.           转移群主\n" << RESET;
    std::cout << GREEN << "12.          返回功能菜单\n" << RESET;
    std::cout << GREEN << "13.          退出登录\n" << RESET;
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
    std::regex pattern(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");

    return std::regex_match(email, pattern);
}
std::string chatclient::getaccount(std::string name){
    j["cmd"]="getaccount";
    j["name"] = name;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return "";
    }
    return res["data"];
}
std::string chatclient::getname(std::string account){
    j["cmd"] = "getname";
    j["account"] = account;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return "";
    }
    return res["data"];
}
bool chatclient::is_existsname(std::string name1){
    j["cmd"]="isexistsname";
    j["name"]=name1;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res;
    if (!wait_future(f, res)) {
        std::cout << "服务器响应超时" << std::endl;
        return false;
    }
    if (res["cmd"] == "cancel") {
        return "";
    }
    if(res["code"]=="1"){
        return true;
    } else {
        return false;
    }
}
bool chatclient::groupinput(){
    char* line = readline(PURPLE "请输入要管理的群聊名称:" RESET);
    if (!inputvail(line)) {
        return false;
    }
    groupname = line;
    free(line);
    if (groupname.empty()) {
        std::cout << PURPLE << "群名不能为空" << std::endl;
        return false;
    }
    bool b = is_existsgroup(groupname);
    if (!b) {
        std::cout << "该群聊并不存在" << std::endl;
        return false;
    }
    b = is_groupmember2(groupname);
    if (!b) {
        std::cout << "您并不在该群当中，无管理权限" << std::endl;
        return false;
    }
    b = is_manager(groupname, account);
    if (!b) {
        std::cout << "您并不是该群群主或者管理员，无管理权限" << std::endl;
        return false;
    }
    printfmembers();
}
void chatclient::fileloadinput(std::vector<std::string> pathfile,std::string& newfilename){
    for (auto t : pathfile) {
        if (t == filename) {
            std::cout << PURPLE << "你的本地路径" << filepath
                      << "里已经存在文件" << t << ",要替换/下载" << std::endl;
            std::string ss;
           char* line = readline(PURPLE "请输入替换y|Y,下载n|N:" RESET);
            if (!inputvail(line)) {
                return;
            }
            ss = line;
            free(line);
            int nn = 1;
            while (running) {
                if (ss == "y" || ss == "Y") {
                    break;
                } else if (ss == "n" || ss == "N") {
                    newfilename = filename;
                    size_t pos = filename.find_last_of('.');
                    std::string name;
                    std::string type;
                    if (pos != std::string::npos) {
                        name = filename.substr(0, pos);
                        type = filename.substr(pos);
                    } else {
                        name = filename;
                        type = "";
                    }
                    while (std::find(pathfile.begin(), pathfile.end(),
                                     newfilename) != pathfile.end()) {
                        newfilename =
                            name + "(" + std::to_string(nn++) + ")" + type;
                    }
                    break;
                } else {
                    line = readline(PURPLE "请输入替换y|Y,下载n|N:" RESET);
                    if (!inputvail(line)) {
                        return;
                    }
                    ss = line;
                    free(line);
                }
            }
        }
    }
}
bool chatclient::resfuture(std::string idd,json j,json& res){
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[idd] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    if (!wait_future(f, res)) {
        std::cout << "服务器响应超时" << std::endl;
        return false;
    }
    if (res["cmd"] == "cancel") {
        return false;
    }
    return true;
}
bool chatclient::inputfilepath(){
    char* line = readline(PURPLE "请输入您要上传的文件路径:" GREEN);
    if (!inputvail(line)) {
        return false;
    }
    filepath = line;
    free(line);
    if (filepath.empty()) {
        std::cout << PURPLE << "上传文件路径为空，文件打开失败!" << RESET
                  << std::endl;
        for (int i = 0; i < 75; i++) {
            std::cout << "-";
        }
        std::cout << std::endl;
        return false;
    }
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cout << "文件打开失败" << std::endl;
        for (int i = 0; i < 75; i++) {
            std::cout << "-";
        }
        std::cout << std::endl;
        return false;
    }
    struct stat st;

    if (stat(filepath.c_str(), &st) == -1) {
        std::cout << "文件不存在\n";
        for (int i = 0; i < 75; i++) {
            std::cout << "-";
        }
        std::cout << std::endl;
        return false;
    }

    filesize = st.st_size;
    close(fd);
    char buf[PATH_MAX];
    strcpy(buf, filepath.c_str());
    filename = basename(buf);
    return true;
}
int chatclient::numvail(int total){
    char* line = readline(PURPLE "请选择你要处理的编号:" RESET);
    int num;
    if (!inputvail(line)) {
        return -1;
    }
    std::string reads = line;
    free(line);
    num = changenum(reads);
    while (num == -1) {
        line = readline(PURPLE "非法输入，请重新输入:" RESET);
        if (!inputvail(line)) {
            return -1;
        }
        reads = line;
        free(line);
        num = changenum(reads);
    }
    while ((num > total || num < 1)) {
        line = readline(PURPLE "请输入要继续上传的文件消息编号!请选择:" RESET);
        if (!inputvail(line)) {
            return -1;
        }
        reads = line;
        free(line);
        num = changenum(reads);
        while (num == -1) {
            line = readline(PURPLE "非法输入，请重新输入:" RESET);
            if (!inputvail(line)) {
                return -1;
            }
            reads = line;
            free(line);
            num = changenum(reads);
        }
    }
    return num;
}
void chatclient::chatinput(bool inchat,std::string name,std::string targetname,std::string targetccount){
    if(inchat){
        chatmessage[targetname] = 0;
    }else{
        groupmessage[targetname] = 0;
    }
    system("clear");
    std::cout << GREEN << "欢迎进入 [" << targetname << "]的聊天界面"
              << RESET << std::endl;
    std::cout << GREEN << "最近十条历史聊天:" << RESET << std::endl;
    std::vector<std::string> chat;
    if (inchat) {
        chat = SQ.getfriendchat(name, chatfriendname);
    } else {
        chat = SQ.getgroupchat(groupname);
    }
    if (chat.empty()) {
        if(inchat){
            handle_getfriendchat(current_chat);
            chat = SQ.getfriendchat(name, chatfriendname);
        }else{
            getgrouphistory(groupname);
            chat = SQ.getgroupchat(groupname);
        }
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
    std::cout << GREEN << "开启新的聊天!" << RESET << std::endl;
    if (!chatuploads[targetname].empty()) {
        std::cout << PURPLE << chatuploads[targetname].size()
                  << "个文件上传失败，ctrl+/选择继续上传文件" << std::endl;
    }
    if (!chatdownloads[targetname].empty()) {
        std::cout << PURPLE << chatdownloads[targetname].size()
                  << "个文件下载失败，ctrl+/选择继续下载文件" << std::endl;
    }
    std::cout << GREEN << "按下ctrl+/选择功能" << std::endl;
    std::string s1 = GREEN;
    s1 += "[" + name + "]:" + RESET;
    while (inchat||groupchat) {
        std::string msg;
        if (shortchat) {
            nt = oldt;
            nt.c_lflag &= ~ECHO;
            nt.c_lflag &= ~ICANON;
            nt.c_cc[VMIN] = 1;
            nt.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSANOW, &nt);
            std::cout << "\033[?2004h" << std::flush;
            msg.clear();
            std::cout << s1 << std::flush;
            while (shortchat) {
                char c;
                int n = read(STDIN_FILENO, &c, 1);
                if (n < 0 && errno == EINTR) {
                    break;
                }
                if (n < 0) {
                    break;
                }
                if (c == '\007') {
                    std::cout << "取消选择" << std::endl;
                    break;
                }
                if (c == 0x1f) {
                    int index = 0;
                    showchatmenu(0);
                    char p;
                    while (running) {
                        read(STDIN_FILENO, &p, 1);
                        char p1, p2;
                        if (p == '\033') {
                            read(STDIN_FILENO, &p1, 1);
                            read(STDIN_FILENO, &p2, 1);
                            if (p1 == '[' && p2 == 'A') {
                                clearchatmenu();
                                index--;
                                if (index < 0) {
                                    index = 8;
                                }
                                showchatmenu(index);
                            }
                            if (p1 == '[' && p2 == 'B') {
                                clearchatmenu();
                                index++;
                                if (index > 8) {
                                    index = 0;
                                }
                                showchatmenu(index);
                            }
                        }
                        if (p == '\n') {
                            clearchatmenu();
                            if (index == 0) {
                                shortchat = true;
                                break;
                            }
                            if (index == 1) {
                                tcflush(STDIN_FILENO, TCIFLUSH);
                                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                                std::cout << "\033[?2004l" << std::flush;
                                std::cout << "\033[G";
                                std::cout << "\033[2K";
                                shortchat = false;
                                break;
                            }
                            if (index == 2) {
                                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                                if(inchat){
                                handle_chatsendfile(current_chat);
                                }else{
                                    handle_chatgroupsendfile(groupname);
                                }
                                std::cout << s1 << std::flush;
                                tcsetattr(STDIN_FILENO, TCSANOW, &nt);
                                break;
                            }
                            if (index == 3) {
                                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                                if(inchat){
                                    handle_chatloadfile(current_chat);
                                }else{
                                    handle_chatgrouploadfile(groupname);
                                }
                                std::cout << "\033[G";
                                std::cout << "\033[2K";
                                std::cout << s1 << std::flush;
                                tcsetattr(STDIN_FILENO, TCSANOW, &nt);
                                break;
                            }
                            if (index == 4) {
                                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                                if(inchat){
                                    handle_rechatsendfile(current_chat);
                                }else{
                                    handle_regroupchatsendfile(groupname);
                                }
                                std::cout << "\033[G";
                                std::cout << "\033[2K";
                                std::cout << s1 << std::flush;
                                tcsetattr(STDIN_FILENO, TCSANOW, &nt);
                                break;
                            }
                            if (index == 5) {
                                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                                if(inchat){
                                    handle_rechatloadfile(current_chat);
                                }else{
                                    handle_regroupchatloadfile(groupname);
                                }
                                std::cout << "\033[G";
                                std::cout << "\033[2K";
                                std::cout << s1 << std::flush;
                                tcsetattr(STDIN_FILENO, TCSANOW, &nt);
                                break;
                            }
                            if (index == 6) {
                                std::cout << GREEN << "所有聊天记录:" << RESET
                                          << std::endl;
                                for (int i = 0; i < chat.size(); i++) {
                                    std::cout << chat[i] << std::endl;
                                }
                                std::cout << "\033[G";
                                std::cout << "\033[2K";
                                std::cout << GREEN << "请继续聊天:" << RESET
                                          << std::endl;
                                std::cout << s1 << std::flush;
                                break;
                            }
                            if (index == 7) {
                                system("clear");
                                std::cout << GREEN << "按ctrl+/选择功能"
                                          << RESET << std::endl;
                                std::cout << s1 << std::flush;
                                break;
                            }
                            if (index == 8) {
                                inchat = false;
                                shortchat = false;
                                tcflush(STDIN_FILENO, TCIFLUSH);
                                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                                system("clear");
                                break;
                            }
                            break;
                        }
                    }
                } else if (c == 127 || c == '\b') {
                    if (!msg.empty()) {
                        bool chinese = false;
                        if ((unsigned char)msg.back() >= 0x80)
                            chinese = true;
                        erase_utf8(msg);
                        if (chinese) {
                            std::cout << "\b \b\b \b";
                        } else {
                            std::cout << "\b \b";
                        }
                        std::cout.flush();
                    }
                    continue;
                } else if (c != '\033') {
                    if (c == '\n') {
                        std::cout << '\n' << std::flush;
                        break;
                    }
                    msg += c;
                    std::cout << c << std::flush;
                    continue;
                } else {
                    std::string ss;
                    ss += c;
                    char x;
                    while (ss.size() < 6) {
                        read(STDIN_FILENO, &x, 1);
                        ss += x;
                        if (x == '~') {
                            break;
                        }
                    }
                    if (ss == "\033[200~") {
                        std::string ssend = "\033[201~";
                        char c1;
                        while (running) {
                            read(STDIN_FILENO, &c1, 1);
                            if (c1 == '\n') {
                                if (msg.empty()) {
                                    continue;
                                }
                                std::cout << msg << std::endl;
                                std::cout << s1 << std::flush;
                                json chat;
                                if(inchat){
                                    chat["cmd"] = "friendchat";
                                    chat["from"] = account;
                                    chat["to"] = current_chat;
                                    chat["message"] = msg;
                                    if (isblocklist[chatfriendname]) {
                                        std::cout << PURPLE
                                                  << "你当前已经被用户拉黑，请"
                                                     "退出聊天"
                                                  << std::endl;
                                    } else {
                                        chat_conn->send(chat.dump() + '\n');
                                    }
                                }else{
                                    chat["cmd"] = "groupchat";
                                    chat["account"] = account;
                                    chat["groupname"] = groupname;
                                    chat["message"] = msg;
                                    if (!is_groupmember2(groupname)) {
                                        std::cout
                                            << PURPLE
                                            << "你当前并不在该群聊，请退出聊天"
                                            << std::endl;
                                    } else {
                                        chat_conn->send(chat.dump() + '\n');
                                    }
                                }
                                msg.clear();
                            } else {
                                msg += c1;
                                size_t end_pos = msg.find(ssend);
                                if (end_pos != std::string::npos) {
                                    msg.erase(end_pos);
                                    std::cout << msg << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            std::cout << "\033[?2004l" << std::flush;
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        } else {
            char *line = readline(s1.c_str());
            if (chat_menu_index != -1) {
                int index = chat_menu_index;
                chat_menu_index = -1;
                if (line != nullptr) {
                    free(line);
                }
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                if (index == 0) {
                    shortchat = true;
                    continue;
                }
                if (index == 1) {
                    shortchat = false;
                    std::cout << "\033[G";
                    std::cout << "\033[2K";
                    continue;
                }
                if (index == 2) {
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    std::cout << "\033[?2004l" << std::flush;
                    if (inchat) {
                        handle_chatsendfile(current_chat);
                    } else {
                        handle_chatgroupsendfile(groupname);
                    }
                    tcflush(STDIN_FILENO, TCIFLUSH);
                    continue;
                }
                if (index == 3) {
                    tcflush(STDIN_FILENO, TCIFLUSH);
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    if (inchat) {
                        handle_chatloadfile(current_chat);
                    } else {
                        handle_chatgrouploadfile(groupname);
                    }
                    tcflush(STDIN_FILENO, TCIFLUSH);
                    continue;
                }
                if (index == 4) {
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    if (inchat) {
                        handle_rechatsendfile(current_chat);
                    } else {
                        handle_regroupchatsendfile(groupname);
                    }
                    std::string s1 = GREEN;
                    s1 += "[" + name + "]:" + RESET;
                    std::cout << s1 << std::flush;
                    tcflush(STDIN_FILENO, TCIFLUSH);
                    continue;
                }
                if (index == 5) {
                    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                    if (inchat) {
                        handle_rechatloadfile(current_chat);
                    } else {
                        handle_regroupchatloadfile(groupname);
                    }
                    std::string s1 = GREEN;
                    s1 += "[" + name + "]:" + RESET;
                    std::cout << s1 << std::flush;
                    tcflush(STDIN_FILENO, TCIFLUSH);
                    continue;
                }
                if (index == 6) {
                    std::cout << GREEN << "所有聊天记录:" << RESET << std::endl;
                    for (int i = 0; i < chat.size(); i++) {
                        std::cout << chat[i] << std::endl;
                    }
                    std::cout << GREEN << "请继续聊天:" << RESET << std::endl;
                    std::cout  << s1 << std::flush;
                    continue;
                }
                if (index == 7) {
                    system("clear");
                    continue;
                }
                if (index == 8) {
                    inchat = false;
                    groupchat = false;
                    shortchat = false;
                    rl_done = 1;
                    system("clear");
                    continue;
                }
                rl_pending_input = 0;
                rl_replace_line("", 0);
                rl_on_new_line();
                tcflush(STDIN_FILENO, TCIFLUSH);
                continue;
            }
            if (line == nullptr) {
                stop1 = true;
                if (chatis_login) {
                    handle_exitlogin();
                }
                handle_exit();
                return;
            }
            if (line[0] == '\0' && cancelinput) {
                std::cout << "取消选择" << std::endl;
                free(line);
                cancelinput = false;
                continue;
            }
            msg = line;
            free(line);
        }
        if (!msg.empty()) {
            json chat;
            if(inchat){
                chat["cmd"] = "friendchat";
                chat["from"] = account;
                chat["to"] = current_chat;
                chat["message"] = msg;
                if (isblocklist[chatfriendname]) {
                    std::cout << PURPLE
                              << "你当前已经被用户拉黑，请"
                                 "退出聊天"
                              << std::endl;
                } else {
                    chat_conn->send(chat.dump() + '\n');
                }
            }else{
                chat["cmd"] = "groupchat";
                chat["account"] = account;
                chat["groupname"] = groupname;
                chat["message"] = msg;
                if (!is_groupmember2(groupname)) {
                    std::cout << PURPLE << "你当前并不在该群聊，请退出聊天"
                              << std::endl;
                } else {
                    chat_conn->send(chat.dump() + '\n');
                }
            }
        }
    }
}
void chatclient::handle_signup() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        char* line = readline(PURPLE "请输入你的邮箱:" RESET);
        if (!inputvail(line)) {
            return;
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if(!is_email){
            std::cout << PURPLE << "邮箱格式不正确" << RESET << std::endl;
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
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return;
        }
        std::cout << std::endl;
        line = readline(PURPLE "请输入你的昵称:" RESET);
        if (!inputvail(line)) {
            return;
        }
        name= line;
        free(line);
        while (is_existsname(name)) {
            line =
                readline(PURPLE "该昵称已被用户使用，请重新输入昵称:" RESET);
            if (!inputvail(line)) {
                return;
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << res["data"] << std::endl;
    }
}
bool chatclient::is_online(std::string account){
    j["cmd"]="is_online";
    j["account"]=account;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return false;
    }
    if(res["code"]=="1"){
        return true;
    }else{
        return false;
    }
}
void chatclient::handle_modifyname(){
    char*line=readline(GREEN"请输入你的新昵称：");
    if (!inputvail(line)) {
        return;
    }
    std::string newname=line;
    free(line);
    while (is_existsname(newname)) {
        if(newname!=name){
            line = readline(PURPLE "该昵称已被用户使用，请重新输入昵称:" RESET);
        }else{
            line = readline(PURPLE "新昵称不能和原昵称一样，请重新输入昵称:" RESET);
        }
        if (!inputvail(line)) {
            return;
        }
        newname = line;
        free(line);
    }
    j["cmd"]="modifyname";
    j["account"]=account;
    j["name"]=newname;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::cout << PURPLE << res["data"] << RESET << std::endl;
    name = newname;
}
bool chatclient::inputemail(){
    char* line = readline(PURPLE "请先输入你的邮箱:" RESET);
    if (!inputvail(line)) {
        return false;
    }
    account = line;
    free(line);
    bool is_email = isQQemail(account);
    if (!is_email) {
        std::cout << PURPLE << "邮箱格式不正确" << RESET << std::endl;
        return false;
    }
    if (!is_exists(account)) {
        std::cout << PURPLE << "该账号并不存在，请先注册!" << RESET
                  << std::endl;
        return false;
    } else {
        if (is_online(account)) {
            std::cout << PURPLE << "该账号正处于登录状态!" << RESET
                      << std::endl;
            return false;
        }
    }
    return true;
}
void chatclient::handle_login_code() {
        if(!inputemail()){
            return;
        }
        j["cmd"] = "verifycode";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        if (res["code"] == "0") {
            std::cout << PURPLE << res["data"] << RESET << std::endl;
            return;
        }
        std::cout << "验证码已经发送到您的邮箱\n" << std::flush;
       char* line = readline(PURPLE "请输入验证码:" RESET);
        if (!inputvail(line)) {
            return;
        }
        verifycode = line;
        free(line);
        j["cmd"] = "verifycodesignin";
        j["account"] = account;
        j["code"] = verifycode;
        id = gen_req_id();
        j["request_id"] = id;
        res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::string cc = res["code"];
        if (cc == "1") {
            chatis_login = true;
            name = getname(account);
            system("clear");
            SQ.open(name);
            handle_uploadcheck();
            handle_downloadcheck();
            handle_chatuploadcheck();
            handle_chatdownloadcheck();
            handle_downfilelist();
        } else {
            std::cout << res["data"] << std::endl;
        }
}
void chatclient::handle_login_key() {
        if (!inputemail()) {
            return;
        }
        password = cinkey();
        if (password.empty()) {
            std::cout << PURPLE << "密码不能为空" << RESET << std::endl;
            tcsetattr(STDIN_FILENO, TCSADRAIN, &oldt);
            return;
        }
        j["cmd"] = "keysignin";
        j["account"] = account;
        j["password"] = password;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::string cc = res["code"];
        if (cc == "1") {
            chatis_login = true;
            name = getname(account);
            system("clear");
            SQ.open(name);
            handle_uploadcheck();
            handle_downloadcheck();
            handle_chatuploadcheck();
            handle_chatdownloadcheck();
            handle_downfilelist();
            handle_chatdownfilelist();
        } else {
            std::cout << res["data"] << std::endl;
        }
}
void chatclient::handle_downfilelist(){
    j["account"] = account;
    j["cmd"]="getdownfilelist";
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    sendfilelist=res["data"];
}
void chatclient::handle_chatdownfilelist(){
    j["account"]=account;
    j["cmd"]="getchatdownfilelist";
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::vector<json> data=res["data"];
    for (int i = 0; i < data.size();i++){
        chatfile[data[i]["from"]].push_back(data[i]);
    }
}
void chatclient::handle_uploadcheck(){
    j["cmd"]="uploadcheck";
    j["account"]=account;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::vector<json> data=res.value("data",std::vector<json>{});
    if(!data.empty()){
        std::cout << PURPLE << "你有" << data.size() << "个文件上传失败"
                  << std::endl;
        for (int i = 0; i < data.size();i++){
            double cur = std::stoll(data[i]["upsize"].get<std::string>()) / 1000000.0;
            double total = std::stoll(data[i]["total"].get<std::string>()) / 1000000.0;
            std::cout <<std::fixed<<std::setprecision(1) << "[" << i + 1 << "] " << data[i]["filename"]
                      << "    上传" << cur << "/"
                      << total<<" MB" << std::endl;
            filestatus f;
            f.sended=data[i]["upsize"];
            f.total=data[i]["total"];
            f.filename=data[i]["filename"];
            f.id=data[i]["fileid"];
            f.to = data[i]["reciver"];
            uploads.push_back(f);
        }
        std::cout << "请稍候在文件菜单中查看详细" << RESET<<std::endl;
    }
}
void chatclient::handle_chatuploadcheck(){
    j["cmd"]="chatuploadcheck";
    j["account"]=account;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id,j, res)) {
        return;
    }
    std::vector<json> data = res.value("data", std::vector<json>{});
    if(!data.empty()){
        std::cout<<"你有"<<data.size()<<"个聊天文件上传失败"<<std::endl;
        for(int i=0;i<data.size();i++){
            double cur =
                std::stoll(data[i]["upsize"].get<std::string>()) / 1000000.0;
            double total =
                std::stoll(data[i]["total"].get<std::string>()) / 1000000.0;
            std::string toname;
            if(!is_exists(data[i]["reciver"])){
                toname = data[i]["reciver"];
            }else{
                toname = getname(data[i]["reciver"]);
            }
            std::cout<<std::fixed<<std::setprecision(1)<<"["<<i+1<<"]   发给"<<toname<<"的"<<data[i]["filename"]<<"  上传"<<cur<<"/"<<total<<"   MB"<<std::endl;
            json jj;
            jj["ID"]=data[i]["fileid"];
            jj["sended"] = data[i]["upsize"];
            jj["total"]=data[i]["total"];
            jj["filename"]=data[i]["filename"];
            chatuploads[toname].push_back(jj);
        }
        std::cout << "请稍候进入用户聊天界面查看详细" << RESET << std::endl;
    }
}
void chatclient::handle_chatdownloadcheck(){
    j["cmd"]="chatdownloadcheck";
    j["account"]=account;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::vector<json> data = res.value("data", std::vector<json>{});
    if(!data.empty()){
        std::cout << PURPLE << "你有" << data.size() << "个聊天文件下载失败"
                  << std::endl;
        for (int i = 0; i < data.size(); i++) {
            double cur =
                std::stoll(data[i]["recived"].get<std::string>()) / 1000000.0;
            double total =
                std::stoll(data[i]["total"].get<std::string>()) / 1000000.0;
            std::string fromname;
            if(!is_exists(data[i]["from"])){
                fromname=data[i]["from"];
            }else{
                fromname = getname(data[i]["from"]);
            }
            std::cout << std::fixed << std::setprecision(1) << "[" << i + 1
                      << "]   " << fromname
                      << "的文件:" << data[i]["filename"] << "    下载" << cur
                      << "/" << total << " MB" << std::endl;
            json jj;
            jj["recived"]=data[i]["recived"];
            jj["total"] = data[i]["total"];
            jj["filename"]=data[i]["filename"];
            jj["ID"] = data[i]["fileid"];
            chatdownloads[fromname].push_back(jj);
        }
        std::cout << "请稍候进入用户聊天界面查看详细" << RESET << std::endl;
    }
}
void chatclient::handle_downingfile(){
   if(downloads.empty()){
       std::cout << PURPLE << "当前并没有下载失败文件" << RESET << std::endl;
       return;
   }
   for (int i = 0; i < downloads.size();i++){
       std::cout << PURPLE << "[" << i + 1 << "]    来自"<<downloads[i].from<<" 的" << downloads[i].filename
                 << "  下载" << downloads[i].recived << "/"
                 << downloads[i].total << std::endl;
   }
   int num = numvail(downloads.size());
   if(num==-1){
       return;
   }
   std::string fileid = downloads[num - 1].id;
   filename = downloads[num - 1].filename;
   std::string from = downloads[num - 1].from;
   std::string filesize = downloads[num - 1].total;
   j["cmd"]="redownloadfile";
   j["account"] = account;
   j["ischat"] = "0";
   j["fileid"] = fileid;
   j["from"] = from;
   j["filename"] = filename;
   id = gen_req_id();
   j["request_id"] = id;
   json res;
   if (!resfuture(id, j, res)) {
       return;
   }
   std::string newfilename = res["newfilename"];
   filename = res["filename"];
   std::string downfilepath = res["filepath"];
   std::string downsize = res["downsize"];
   int n = fileclient_->reloadfile(filename,newfilename, filesize, fileid, downfilepath,downsize,false);
   downloads.erase(downloads.begin() + num - 1);
}
void chatclient::handle_downloadcheck(){
    j["cmd"] = "downloadcheck";
    j["account"] = account;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::vector<json> data = res.value("data", std::vector<json>{});
    if (!data.empty()) {
        std::cout << PURPLE << "你有" << data.size() << "个文件下载失败"
                  << std::endl;
        for (int i = 0; i < data.size(); i++) {
            double cur =
                std::stoll(data[i]["downsize"].get<std::string>()) / 1000000.0;
            double total =
                std::stoll(data[i]["filesize"].get<std::string>()) / 1000000.0;
            std::cout << std::fixed << std::setprecision(1) << "[" << i + 1
                      << "]     来自"<<data[i]["sender"] <<"  "<< data[i]["filename"] << "  下载" << cur << "/"
                      << total << " MB" << std::endl;
            filestatus f;
            f.recived = data[i]["downsize"];
            f.total = data[i]["filesize"];
            f.filename = data[i]["filename"];
            f.id = data[i]["fileid"];
            f.from = data[i]["sender"];
            downloads.push_back(f);
        }
        std::cout << "请稍候再文件菜单中查看详细" << RESET << std::endl;
    }
}
void chatclient::handle_forget_key() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        char* line = readline(PURPLE "请先输入你的邮箱:" RESET);
        if (!inputvail(line)) {
            return;
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if (!is_email) {
            std::cout << PURPLE << "邮箱格式不正确" << RESET << std::endl;
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << res["data"] << std::endl;
    }
}
void chatclient::handle_destory() {
    if (chatis_login) {
        std::cout << "请重新输入6-15之间数字!" << std::endl;
    } else {
        char* line = readline(PURPLE "请先输入你的邮箱:" RESET);
        if (!inputvail(line)) {
            return;
        }
        account = line;
        free(line);
        bool is_email = isQQemail(account);
        if (!is_email) {
            std::cout << PURPLE << "邮箱格式不正确" << RESET << std::endl;
            return;
        }
        if (!is_exists(account)) {
            std::cout << PURPLE << "该账号并不存在，请先注册!" << RESET
                      << std::endl;
            return;
        }
        line = readline(PURPLE "请输入您的密码:" RESET);
        if (!inputvail(line)) {
            return;
        }
        password = line;
        free(line);
        j["cmd"] = "destory";
        j["account"] = account;
        j["password"] = password;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}

void chatclient::handle_exit() {
    std::cout << std::endl;
    std::cout<<PURPLE << "再见！" <<RESET<< std::endl;
    cancel_requests();
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
    char* line = readline(PURPLE "请输入要添加好友的账号:" RESET);
    if (!inputvail(line)) {
        return;
    }
    frienduser = line;
    free(line);
    bool is_email = isQQemail(frienduser);
    if (!is_email) {
        std::cout << PURPLE << "邮箱格式不正确" << RESET << std::endl;
        return;
    }
    if (frienduser == account) {
        std::cout << PURPLE << "不可以添加自己为好友" << RESET << std::endl;
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << PURPLE << res["data"] << RESET << std::endl;
    }
}
void chatclient::handle_applyjoingroup() {
        char* line = readline(PURPLE "请输入你要加入群聊的名称:" RESET);
        if (!inputvail(line)) {
            return;
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
        b = is_groupmember2(groupname);
        if (b) {
            std::cout << "您已经是群聊成员了" << std::endl;
            return;
        }
        j["cmd"] = "applyjoingroup";
        j["account"] = account;
        j["groupname"] = groupname;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << PURPLE << res["data"] << RESET << std::endl;
}
void chatclient::handle_exitlogin() {
        json j1;
        j1["cmd"] = "exitlogin";
        j1["account"] = account;
        id = gen_req_id();
        j1["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << PURPLE << res["data"] << RESET << std::endl;
        chatis_login = false;
        system("clear");
        {
            std::lock_guard<std::mutex> lock(event_mutex);

            while (!event_queue.empty())
                event_queue.pop();
        }
        msg_map.clear();
    }
int chatclient::is_friend(std::string friendaccount) {
    j["cmd"] = "is_friend";
    j["account"] = account;
    j["friendaccount"] = friendaccount;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return -1;
    }
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
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
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
void chatclient::erase_utf8(std::string& msg) {
    if (msg.empty()){
        return;
    }
    int i = msg.size() - 1;
    while ((i >= 0) && (((unsigned char)msg[i] & 0xC0) == 0x80) ) {
        i--;
    }
    msg.erase(i);
}
void chatclient::showchatmenu(int index){
    std::vector<std::string> menu = {"  /开启短文本模式",
                                     "  /开启长文本模式",
                                     "  /上传文件",
                                     "  /下载文件",
                                     "  /继续上传文件",
                                     "  /继续下载文件",
                                     "  /查看历史聊天记录",
                                     "  /清屏",
                                     "  /退出聊天"};
    std::cout << std::endl;
    for (int i = 0; i < menu.size(); i++) {
        if (i != menu.size() - 1) {
            if (i == index) {
                std::cout << BG_SELECT << PURPLE << menu[i] << RESET
                                                << std::endl;
            }else{
                std::cout << BG_SELECT << WHITE << menu[i] << RESET
                          << std::endl;
            }
            }else{
                if (i == index) {
                    std::cout << BG_SELECT << PURPLE << menu[i] << RESET
                              << std::flush;
                } else {
                    std::cout << BG_SELECT << WHITE << menu[i] << RESET
                              << std::flush;
                }
            }
    }
                                    }
void chatclient::clearchatmenu()
{
    int size = 9;
    for (int i = 0; i < size; i++) {
        std::cout << "\033[G";
        std::cout << "\033[2K";
        if(i != size-1)
        {
            std::cout << "\033[A";
        }
    }
    std::cout << "\033[A";
    std::cout << "\033[G";
    std::cout << "\033[2K";
    std::string s1 = GREEN;
    s1 += "[" + name + "]:" + RESET;
    std::cout << s1;
    std::cout << std::flush;
}
void chatclient::handle_friendchat() {
    char* line = readline(PURPLE "请输入要私聊的好友名称:" RESET);
    if (!inputvail(line)) {
        return;
    }
     chatfriendname = line;
    free(line);
    if (!is_existsname(chatfriendname)) {
        std::cout << PURPLE << "该用户并不存在!" << RESET << std::endl;
        return;
    }
    current_chat = getaccount(chatfriendname);
    friendhistory.clear();
    int is = is_friend(current_chat);
    if (is == 1) {
        std::cout << "该账号并不存在" << std::endl;
    } else if (is == 0) {
        std::cout << "对方目前还不是您的好友" << std::endl;
    }else if(isblocklist[chatfriendname]){
        std::cout << PURPLE << "你已被该用户拉黑，无法私聊" << RESET
                  << std::endl;
    } else {
        chatinput(true, name, chatfriendname, current_chat);
    }
}
void chatclient::handle_chatloadfile(std::string from){
    std::string newfilename;
    if (chatfile[from].empty()) {
        std::cout << PURPLE << "当前没有好友发来文件要处理!" << RESET
                  << std::endl;
        return;
    }
    for(int i=0;i<75;i++){
        std::cout << "-";
    }
    std::cout << std::endl;
    for (int i = 0; i < chatfile[from].size();i++){
        std::cout << PURPLE << "[" << i + 1 << "] "
                  << chatfile[from][i]["filename"] << std::endl;
    }
    int num = numvail(chatfile[from].size());
    if (num == -1) {
        return;
    }
    std::cout << std::endl;
    char* line = readline(PURPLE "请输入您要下载文件到本地的路径：" RESET);
     if (!inputvail(line)) {
         return;
     }
    filepath = line;
    free(line);
    std::cout << RESET << std::endl;
    filename = chatfile[from][num-1]["filename"];
    std::vector<std::string> pathfile = getlocalfile(filepath);
    int nn = 1;
    fileloadinput(pathfile, newfilename);
    json reply;
    reply["cmd"] = "recvfile";
    reply["from"] = from;
    reply["ischat"] = "1";
    reply["newfilename"] = newfilename;
    reply["account"] = account;
    reply["filepath"] = filepath;
    reply["ID"] = chatfile[from][num-1]["ID"];
    std::string ID = chatfile[from][num-1]["ID"];
    reply["filename"] = filename;
    id = gen_req_id();
    reply["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::string filesize = res["filesize"];
    int n = fileclient_->loadfile(filename, filesize, ID, filepath,true,newfilename);
    for (int i = 0; i < 75;i++){
        std::cout<< "-";
    }
    std::cout << std::endl;
    if (n == -1) {
        j["cmd"]="deldownrecord";
        j["ID"]=ID;
        j["account"] = account;
        j["filepath"] = filepath;
        j["filename"]=filename;
        j["ischat"] = '1';
        chat_conn->send(j.dump() + '\n');
        return;
    }
    json j;
    j["cmd"] = "friendchat";
    j["from"] = account;
    j["to"] = from;
    j["message"] = "我下载了文件" + filename;
    if (isblocklist[chatfriendname]) {
        std::cout << PURPLE
                  << "你当前已经被用户拉黑，请"
                     "退出聊天"
                  << std::endl;
    } else {
        chat_conn->send(j.dump() + '\n');
    }
}
void chatclient::handle_rechatsendfile(std::string toaccount){
    std::string toname = getname(toaccount);
    if(chatuploads[toname].empty()){
        std::cout << PURPLE << "当前没有上传失败的文件" << RESET << std::endl;
        return;
    }
    for (int i = 0; i < 75;i++){
        std::cout << "-";
    }
    std::cout << std::endl;
    for (int i = 0; i < chatuploads[toname].size(); i++) {
        double cur =
            std::stoll(chatuploads[toname][i]["sended"].get<std::string>()) /
            1000000.0;
        double total =
            std::stoll(chatuploads[toname][i]["total"].get<std::string>()) /
            1000000.0;
        std::cout << std::fixed << std::setprecision(1) <<PURPLE<< "[" << i + 1
                  << "]   " << chatuploads[toname][i]["filename"] << "  上传"
                  << cur << "/" << total << "   MB" << std::endl;
    }
    int num = numvail(chatuploads[toname].size());
    if (num == -1) {
        return;
    }
        std::cout << std::endl;
        std::string ID = chatuploads[toname][num - 1]["ID"];
        std::string filename1=chatuploads[toname][num-1]["filename"];
        std::string filesize = chatuploads[toname][num - 1]["total"];
        j["cmd"] = "resendfile";
        j["fileid"]=ID;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::string uploaded = res["uploaded"];
        std::string filepath = res["filepath"];
        int nn=fileclient_->uploadedsendfile(ID, uploaded, filepath, filename1,
                                      filesize, false,true,false);
        for (int i = 0; i < 75;i++){
            std::cout << "-";
        }
        if(nn==-1){

        }else{
            chatuploads[toname].erase(chatuploads[toname].begin() + num - 1);
        }
        std::cout << std::endl;
        json j1;
        j1["cmd"] = "friendchat";
        j1["from"] = account;
        j1["message"] = "[上传文件]:" + filename1;
        j1["to"] = toaccount;
        if (isblocklist[chatfriendname]) {
            std::cout << PURPLE
                      << "你当前已经被用户拉黑，请"
                         "退出聊天"
                      << std::endl;
        } else {
            chat_conn->send(j1.dump() + '\n');
        }
}
void chatclient::handle_rechatloadfile(std::string from1){
    std::string fromname = getname(from1);
    if (chatdownloads[fromname].empty()) {
        std::cout<<PURPLE<<"当前并没有下载失败的文件"<<RESET<<std::endl;
        return;
    }
    for(int i=0;i<chatdownloads[fromname].size();i++){
        double cur =
            std::stoll(chatdownloads[fromname][i]["recived"].get<std::string>()) /
            1000000.0;
        double total =
            std::stoll(chatdownloads[fromname][i]["total"].get<std::string>()) /
            1000000.0;
        std::cout << std::fixed << std::setprecision(1) << PURPLE << "["
                  << i + 1 << "] " << chatdownloads[fromname][i]["filename"]
                  << "    下载" << cur << "/" << total << " MB" << RESET
                  << std::endl;
    }
    int num = numvail(chatdownloads[fromname].size());
    if (num == -1) {
        return;
    }
    std::string fileid=chatdownloads[fromname][num-1]["ID"];
    std::string filename1 = chatdownloads[fromname][num - 1]["filename"];
    std::string filesize=chatdownloads[fromname][num-1]["total"];
    j["cmd"] = "redownloadfile";
    j["account"] = account;
    j["ischat"] = "1";
    j["fileid"] = fileid;
    j["from"]=from1;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::string downfilepath = res["filepath"];
    std::string downsize = res["downsize"];
    std::string newfilename = res["newfilename"];
    filename = res["filename"];
    int n = fileclient_->reloadfile(filename, newfilename, filesize, fileid,
                                    downfilepath, downsize, true);
    chatdownloads[fromname].erase(chatdownloads[fromname].begin() + num - 1);
    for (int i = 0; i < 75; i++) {
        std::cout << "-";
    }
    std::cout << std::endl;
    j["cmd"] = "friendchat";
    j["from"] = account;
    j["to"] = from1;
    j["message"] = "我下载了文件" + filename1;
    chat_conn->send(j.dump() + '\n');
}
void chatclient::handle_regroupchatsendfile(std::string groupname1){
    if(chatuploads[groupname1].empty()){
        std::cout << PURPLE << "当前没有上传失败的文件" << RESET << std::endl;
        return;
    }
    for(int i=0;i<75;i++){
        std::cout << "-";
    }
    std::cout << std::endl;
    for (int i = 0; i < chatuploads[groupname1].size(); i++) {
        double cur =
            std::stoll(chatuploads[groupname1][i]["sended"].get<std::string>()) /
            1000000.0;
        double total =
            std::stoll(chatuploads[groupname1][i]["total"].get<std::string>()) /
            1000000.0;
        std::cout << std::fixed << std::setprecision(1) << "[" << i + 1
                  << "]   " << chatuploads[groupname1][i]["filename"]
                  << "   上传" << cur << "/" << total << " MB" << std::endl;
    }
    int num = numvail(chatuploads[groupname1].size());
    if (num == -1) {
        return;
    }
    std::cout << std::endl;
    std::string ID=chatuploads[groupname1][num-1]["ID"];
    std::string filename1=chatuploads[groupname1][num-1]["filename"];
    std::string filesize=chatuploads[groupname1][num-1]["total"];
    j["cmd"] = "resendfile";
    j["fileid"] = ID;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::string uploaded = res["uploaded"];
    std::string filepath = res["filepath"];
    int n=fileclient_->uploadedsendfile(ID, uploaded, filepath, filename1, filesize, true,
                                  false,true);
    for(int i=0;i<75;i++){
        std::cout << "-";
    }
    std::cout << std::endl;
    chatuploads[groupname1].erase(
        chatuploads[groupname1].begin() + num - 1);
    json j1;
    j1["cmd"] = "groupchat";
    j1["account"] = account;
    j1["groupname"] = groupname1;
    j1["message"] = "[上传文件]:" + filename1;
   if(!is_groupmember2(groupname1)){
       std::cout << PURPLE << "你当前不在该群聊，请退出" << std::endl;
   }else{
       chat_conn->send(j1.dump() + '\n');
   }
}
void chatclient::handle_regroupchatloadfile(std::string groupname1){
    if(chatdownloads[groupname1].empty()){
        std::cout << PURPLE << "当前并没有下载失败的文件" << RESET << std::endl;
        return;
    }
    for(int i=0;i<75;i++){
        std::cout << "-";
    }
    std::cout<<std::endl;
    for(int i=0;i<chatdownloads[groupname1].size();i++){
        double cur =
            std::stoll(
                chatdownloads[groupname1][i]["recived"].get<std::string>()) /
            1000000.0;
        double total =
            std::stoll(chatdownloads[groupname1][i]["total"].get<std::string>()) /
            1000000.0;
        std::cout << std::fixed << std::setprecision(1) << PURPLE << "["
                  << i + 1 << "]    "
                  << chatdownloads[groupname1][i]["filename"] << " 下载" << cur
                  << "/" << total << "   MB" << std::endl;
    }
    int num = numvail(chatdownloads[groupname1].size());
    if (num == -1) {
        return;
    }
    std::string fileid = chatdownloads[groupname1][num - 1]["ID"];
    std::string filename1 = chatdownloads[groupname1][num - 1]["filename"];
    std::string filesize = chatdownloads[groupname1][num - 1]["total"];
    j["cmd"] = "redownloadfile";
    j["account"]=account;
    j["fileid"] = fileid;
    j["ischat"] = "1";
    j["filename"] = filename1;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::string downfilepath = res["filepath"];
    std::string downsize = res["downsize"];
    std::string newfilename=res["newfilename"];
    filename1 = res["filename"];
    int n = fileclient_->reloadfile(filename1, newfilename, filesize, fileid,
                                    downfilepath, downsize, true);
    for(int i=0;i<75;i++){
        std::cout << "-";
    }
    std::cout << std::endl;
    if (n != -1) {
        json j;
        j["cmd"] = "groupchat";
        j["account"] = account;
        j["groupname"] = groupname1;
        j["message"] = "我下载了文件" + filename1;
        chat_conn->send(j.dump() + '\n');
        chatdownloads[groupname1].erase(chatdownloads[groupname1].begin() + num -
                                        1);
    }
}
void chatclient::handle_friendlist() {
        j["cmd"] = "friendlist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << std::endl;
        std::cout << PURPLE << data << RESET << std::endl;
}
void chatclient::printfmembers() {
    j["cmd"] = "groupmember";
    j["account"] = account;
    j["groupname"] = groupname;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
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
    json res;
    if (!resfuture(id, j, res)) {
        return false;
    }
    if (res["code"] == "1") {
        return true;
    } else {
        return false;
    }
}
     bool chatclient::is_groupmember2(std::string groupname) {
    for (auto s : grouplist) {
        if (s == groupname) {
            return true;
        }
    }
    return false;
}
void chatclient::handle_grouplist() {
        j["cmd"] = "grouplist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        size_t pos = 0;
        grouplist = res["data"];
}
void chatclient::printfgrouplist(){
    handle_grouplist();
    std::cout << PURPLE << "你加入的群聊:" << std::endl;
    for (auto s : grouplist) {
        std::cout << PURPLE << s << std::endl;
    }
}
bool chatclient::is_manager(std::string groupname, std::string count) {
    j["cmd"] = "is_manager";
    j["account"] = count;
    j["groupname"] = groupname;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return false;
    }
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
    json res;
    if (!resfuture(id, j, res)) {
        return false;
    }
    if (res["code"] == "1") {
        return true;
    } else {
        return false;
    }
}
void chatclient::handle_setgroupmanager() {
       if(!groupinput()){
           return;
       }
        int num;
       char* line =
            readline(PURPLE "请输入要添加管理员1 / 删除管理员2:" RESET);
        if (!inputvail(line)) {
            return;
        }
        std::string reads = line;
        free(line);
        num = changenum(reads);
        while ((num != 1 && num != 2)) {
            if (num == -1) {
                line = readline(PURPLE "非法输入，请重新输入:" RESET);
            } else {
                line = readline(PURPLE "请输入数字1-2:" RESET);
            }
            if (!inputvail(line)) {
                return;
            }
            reads = line;
            free(line);
            num = changenum(reads);
        }
        if (num == 1) {
            std::string count;
            std::string friendname;
            line = readline(PURPLE "请输入要添加为管理员的用户:" RESET);
            if (!inputvail(line)) {
                return;
            }
            friendname = line;
            free(line);
           bool b = is_existsname(friendname);
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
            if (!inputvail(line)) {
                return;
            }
            friendname = line;
            free(line);
            if (!is_existsname(friendname)) {
                std::cout << PURPLE << "该用户并不存在!" << RESET << std::endl;
                return;
            }
            count = getaccount(friendname);
           bool b = is_groupmember(groupname, count);
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << PURPLE << res["data"] << RESET << std::endl;
}
void chatclient::handle_groupmember() {
        char* line = readline(PURPLE "请输入要查看群成员的群聊名称:" RESET);
        if (!inputvail(line)) {
            return;
        }
        groupname = line;
        if (groupname.empty()) {
            std::cout << PURPLE << "群名不能为空" << std::endl;
            return;
        }
        free(line);
        printfmembers();
}
void chatclient::handle_blocklist() {
        j["cmd"] = "blocklist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << PURPLE << data << RESET << std::endl;
}
void chatclient::handle_onlinelist() {
        j["cmd"] = "onlinelist";
        j["account"] = account;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << PURPLE << data << RESET << std::endl;
}
void chatclient::handle_block() {
        std::string friendname;
        char* line = readline(PURPLE "请输入要拉黑的好友:" RESET);
        if (!inputvail(line)) {
            return;
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
            json res;
            if (!resfuture(id, j, res)) {
                return;
            }
            std::cout << res["data"] << std::endl;
        }
}
int chatclient::is_blockfriend(std::string friendaccount) {
    json j1;
    j1["cmd"] = "is_block";
    j1["account"] = account;
    j1["friendaccount"] = friendaccount;
    id = gen_req_id();
    j1["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return -1;
    }
    if (res["data"] == "1") {
        return 1;
    } else if (res["data"] == "2") {
        return 2;
    } else {
        return 0;
    }
}
bool chatclient::is_blocked(std::string frienduser){
    j["cmd"]="is_blocked";
    j["account"]=account;
    j["friendacount"]=frienduser;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    chat_conn->send(j.dump() + '\n');
    json res;
    if (!wait_future(f, res)) {
        std::cout << "服务器响应超时" << std::endl;
        return -1;
    }
    if (res["cmd"] == "cancel") {
        return false;
    }
    if(res["code"]=="1"){
        return true;
    }else{
        return false;
    }
}
bool chatclient::is_exists(std::string account){
    j["cmd"]="isexists";
    j["account"] = account;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return false;
    }
    if(res["code"]=="1"){
        return true;
    }else{
        return false;
    }
}
void chatclient::handle_ownergrouplist(){
        j["cmd"]="ownergrouplist";
        j["account"]=account;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        size_t pos = 0;
        std::string data = res["data"];
        while ((pos = data.find("\\n", pos)) != std::string::npos) {
            data.replace(pos, 2, "\n");
            pos += 1;
        }
        std::cout << PURPLE << data << RESET << std::endl;
}
bool chatclient::is_owner(std::string account,std::string groupname){
    j["cmd"]="is_owner";
    j["account"]=account;
    j["groupname"]=groupname;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return false;
    }
    if(res["code"]=="1"){
        return true;
    }else{
        return false;
    }
}
void chatclient::handle_changeowner(){
    char *line=readline(PURPLE"请输入你要转移群组的群聊名称:"RESET);
    if (!inputvail(line)) {
        return;
    }
    groupname=line;
    free(line);
    bool b=is_existsgroup(groupname);
    if(!b){
        std::cout << PURPLE << "该群聊并不存在" << RESET << std::endl;
        return;
    }
    b=is_owner(account,groupname);
    if(!b){
        std::cout<<PURPLE<<"你并不是该群群主，没有转移群主的权限!"<<RESET<<std::endl;
        return;
    }
    line = readline(PURPLE "请输入你要转移群主权限的群成员:" RESET);
    if (!inputvail(line)) {
        return;
    }
    std::string friendname = line;
    free(line);
    b=is_existsname(friendname);
    if(!b){
        std::cout << PURPLE << "该用户帐号并不存在" << RESET << std::endl;
        return;
    }
    frienduser = getaccount(friendname);
    b = is_groupmember(groupname, frienduser);
    if(!b){
        std::cout << PURPLE << "该用户并不是群成员,无法转移群聊" << RESET
                  << std::endl;
        return;
    }
    j["cmd"]="changeowner";
    j["groupname"]=groupname;
    j["owner"] = account;
    j["friendaccount"] = frienduser;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::cout << GREEN << res["data"] << std::endl;
}
void chatclient::handle_disblock() {
        std::string friendname;
        char* line = readline(PURPLE "请输入要取消拉黑的好友:" RESET);
        if (!inputvail(line)) {
            return;
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
            json res;
            if (!resfuture(id, j, res)) {
                return;
            }
            std::cout << PURPLE << res["data"] << RESET << std::endl;
        }
}
void chatclient::handle_delfriend() {
        std::string friendname;
        char* line = readline(PURPLE "请输入要删除的好友:" RESET);
        if (!inputvail(line)) {
            return;
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << PURPLE << res["data"] << RESET << std::endl;
}
void chatclient::handle_creategroup() {
        char* line = readline(PURPLE "请输入要创建的群聊的名字:" RESET);
        if (!inputvail(line)) {
            return;
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << res["data"] << std::endl;
}
void chatclient::handle_exitgroup() {
        char* line = readline(PURPLE "请输入要退出的群聊的名称:" RESET);
        if (!inputvail(line)) {
            return;
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << PURPLE << res["data"] << RESET << std::endl;
}
void chatclient::handle_delmember() {
    if (!groupinput()) {
        return;
    }
    std::string friendname;
    std::string count;
   char* line = readline(PURPLE "请输入您要删除的成员:" RESET);
    if (!inputvail(line)) {
        return;
    }
    friendname = line;
    free(line);
    if (!is_existsname(friendname)) {
        std::cout << PURPLE << "该用户并不存在" << RESET << std::endl;
        return;
    }
    count = getaccount(friendname);
   bool b = is_groupmember(groupname, count);
    if (!b) {
        std::cout << "该用户并不是群聊成员" << std::endl;
        return;
    }
    j["cmd"] = "delmember";
    j["account"] = count;
    j["groupname"] = groupname;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::cout << PURPLE << res["data"] << RESET << std::endl;
}
void chatclient::handle_delgroup() {
        char* line = readline(PURPLE "请输入要解散的群聊名称:" RESET);
        if (!inputvail(line)) {
            return;
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
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        std::cout << res["data"] << std::endl;
}
void chatclient::handle_addfriendmsg() {
    json reply;
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
            int num = numvail(addlist.size());
            if (num == -1) {
                return;
            }
            frienduser = addlist[num - 1]["account"];
            std::string ss;
            std::cout << std::endl;
           char* line = readline(PURPLE "请输入y 同意|n 拒绝:" RESET);
            if (!inputvail(line)) {
                return;
            }
            ss = line;
            free(line);
            num -= 1;
            while(running){
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
                    if (!inputvail(line)) {
                        return;
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
            json res;
            if (!resfuture(id, j, res)) {
                return;
            }
            std::cout << PURPLE << "[系统消息]:" << res["data"] << RESET
                      << std::endl;
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
    while ((entry = readdir(dir)) != nullptr) {
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
       if(sendfilelist.empty()){
           std::cout << PURPLE << "当前并没有文件要处理哦!" << RESET
                     << std::endl;
           return;
       }else{
           std::string newfilename;
           std::cout << GREEN << "文件消息：" << RESET << std::endl;
           for (auto i = 0; i < sendfilelist.size(); i++) {
               std::cout << GREEN << "[" << i + 1 << "]"
                         << sendfilelist[i]["data"] << RESET << std::endl;
           }
           int num = numvail(sendfilelist.size());
           if (num == -1) {
               return;
           }
            char* line =
               readline(PURPLE "请输入您要下载文件到本地的路径：" RESET);
             if (!inputvail(line)) {
                 return;
             }
           filepath = line;
           free(line);
           std::cout << RESET << std::endl;
           std::string from = sendfilelist[num - 1]["from"];
           filename = sendfilelist[num - 1]["filename"];
           std::vector<std::string> pathfile = getlocalfile(filepath);
           fileloadinput(pathfile, newfilename);
           reply["cmd"] = "recvfile";
           reply["newfilename"] = newfilename;
           reply["ischat"] = "0";
           reply["from"] = from;
           reply["account"] = account;
           reply["filepath"] = filepath;
           reply["ID"] = sendfilelist[num - 1]["ID"];
           std::string ID = sendfilelist[num - 1]["ID"];
           reply["filename"] = filename;
           id = gen_req_id();
           reply["request_id"] = id;
           json res;
           if (!resfuture(id, j, res)) {
               return;
           }
           std::string filesize = res["filesize"];
           int n =
               fileclient_->loadfile(filename, filesize, ID, filepath, false,newfilename);
           if (n == -1) {
               j["cmd"] = "deldownrecord";
               j["ID"] = ID;
               j["account"] = account;
               j["filepath"] = filepath;
               j["filename"] = filename;
               j["ischat"] = '0';
               chat_conn->send(j.dump() + '\n');
               return;
           }
       }
}
void chatclient::handle_applyjoinmsg() {
    json reply;
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
           int num = numvail(applyjoinlist.size());
           if (num == -1) {
               return;
           }
           std::cout << std::endl;
           std::string g = applyjoinlist[num - 1]["groupname"];
           std::string a = getname(applyjoinlist[num - 1]["from"]);
           std::cout<<PURPLE << "请处理来自用户:" << a << "加入群聊" << g
                     << "的消息:" << std::endl;
           std::string ss;
           char* line = readline(PURPLE "请输入y 同意|n 拒绝:" RESET);
            if (!inputvail(line)) {
                return;
            }
           ss = line;
           free(line);
           std::string a1 = applyjoinlist[num - 1]["from"];
           applyjoinlist.erase(applyjoinlist.begin() + num - 1);
           while (running) {
               if (ss == "y" || ss == "Y") {
                   reply["cmd"] = "agreejoingroup";
                   break;
               } else if (ss == "n" || ss == "N") {
                   reply["cmd"] = "refusejoingroup";
                   break;
               } else {
                    line = readline(PURPLE "请输入y 同意|n 拒绝:" RESET);
                    if (!inputvail(line)) {
                        return;
                    }
                   ss = line;
                   free(line);
               }
           }
           reply["account"] = a1;
           reply["groupname"] = g;
           id = gen_req_id();
           reply["request_id"] = id;
           json res;
           if (!resfuture(id, j, res)) {
               return;
           }
           std::cout << PURPLE << "[系统消息]:" << res["data"] << RESET
                     << std::endl;
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
    int num = numvail(uploads.size());
    if (num == -1) {
        return;
    }
    std::string fileid = uploads[num - 1].id;
    filename = uploads[num - 1].filename;
    ;
    std::string filesize = uploads[num - 1].total;
    j["cmd"] = "resendfile";
    j["fileid"]=fileid;
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    bool b;
    std::string to = uploads[num-1].to;
    if (!is_exists(to)){
        b = true;
    }else{
        b = false;
    }
        std::string uploaded = res["uploaded"];
    std::string filepath = res["filepath"];
    int n=fileclient_->uploadedsendfile(fileid, uploaded, filepath, filename,
                                  filesize,b,false,false);
    uploads.erase(uploads.begin() + num - 1);
}
void chatclient::getgrouphistory(std::string groupname) {
        j["cmd"] = "getgrouphistory";
        j["groupname"] = groupname;
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
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
void chatclient::handle_groupchat() {
         char* line = readline(PURPLE "请输入要进行群聊的群聊名称:" RESET);
         if (!inputvail(line)) {
             return;
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
         handle_grouplist();
         b = is_groupmember2(groupname);
         if (!b) {
             std::cout << PURPLE << "您当前并不在该群聊里，不能进行聊天"
                       << RESET << std::endl;
             return;
         }
         chatinput(false, name, groupname, "");
}
void chatclient::handle_chatgroupsendfile(std::string groupname1){
    for(int i=0;i<75;i++){
        std::cout << "-";
    }
    std::cout << std::endl;
    if (!inputfilepath()) {
        return;
    }
    json j;
    j["cmd"] = "groupsendfile";
    j["from"] = account;
    j["ischat"] = "1";
    j["groupname"] = groupname1;
    j["filename"] = filename;
    j["filepath"] = filepath;
    j["filesize"] = std::to_string(filesize);
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    if(res["code"]=="1"){
        std::cout << PURPLE << "你并不在该群聊里，不能发送文件" << std::endl;
        return;
    }
    std::string ID = res["ID"];
    fileclient_->groupsendfile(ID, filepath, filename, std::to_string(filesize),
                               true);
    for (int i = 0; i < 75;i++){
        std::cout << "-";
    }
    std::cout << std::endl;
    json j1;
    j1["cmd"]="groupchat";
    j1["account"]=account;
    j1["groupname"] = groupname1;
    j1["message"] = "[上传文件]:" + filename;
  if(!is_groupmember2(groupname1)){
      std::cout << PURPLE << "你并不在群聊里，请退出" << std::endl;
  }else{
      chat_conn->send(j1.dump() + '\n');
  }
}
void chatclient::handle_chatgrouploadfile(std::string groupname2){
    if(chatfile[groupname2].empty()){
        std::cout << PURPLE << "当前并没有群聊文件需要处理!" << RESET
                  << std::endl;
        return;
    }
    std::string newfilename;
    for (int i = 0; i < 75; i++) {
        std::cout << "-";
    }
    std::cout << std::endl;
    for(int i=0;i<chatfile[groupname2].size();i++){
        std::cout << PURPLE << "[" << i + 1
                  << "]:    " << chatfile[groupname2][i]["filename"]
                  << std::endl;
    }
    int num = numvail(chatfile[groupname2].size());
    if (num == -1) {
        return;
    }
    std::cout << std::endl;
   char* line = readline(PURPLE "请输入您要下载文件到本地的路径：" RESET);
    if (!inputvail(line)) {
        return;
    }
    filepath = line;
    free(line);
    std::cout << RESET << std::endl;
    filename = chatfile[groupname2][num - 1]["filename"];
    std::vector<std::string> pathfile = getlocalfile(filepath);
    fileloadinput(pathfile, newfilename);
    json reply;
    reply["cmd"] = "recvfile";
    reply["newfilename"] = newfilename;
    reply["ischat"] = "1";
    reply["from"] = groupname2;
    reply["account"] = account;
    reply["filepath"] = filepath;
    reply["ID"] = chatfile[groupname2][num - 1]["ID"];
    std::string ID = chatfile[groupname2][num - 1]["ID"];
    reply["filename"] = filename;
    id = gen_req_id();
    reply["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    std::string filesize = res["filesize"];
    int n = fileclient_->loadfile(filename, filesize, ID, filepath, true,newfilename);
    for (int i = 0; i < 75; i++) {
        std::cout << "-";
    }
    std::cout << std::endl;
    if (n == -1) {
        j["cmd"] = "deldownrecord";
        j["ID"] = ID;
        j["account"] = account;
        j["filepath"] = filepath;
        j["filename"] = filename;
        j["ischat"] = '1';
        chat_conn->send(j.dump() + '\n');
        return;
    }
    json j;
    j["cmd"] = "groupchat";
    j["account"] = account;
    j["groupname"] = groupname2;
    j["message"] = "我下载了文件" + filename;
    chat_conn->send(j.dump() + '\n');
}
void chatclient::handle_sendfile() {
        handle_friendlist();
        std::string friendname;
        char* line = readline(PURPLE"请输入您要发送文件的用户:"GREEN);
        if (!inputvail(line)) {
            return;
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
        if (!inputfilepath()) {
            return;
        }
        json j;
        j["cmd"] = "sendfile";
        j["from"] = account;
        j["ischat"] = "0";
        j["to"] = frienduser;
        j["filename"] = filename;
        j["filepath"] = filepath;
        j["filesize"] = std::to_string(filesize);
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        if(res["code"]=="1"){
            std::cout << PURPLE << "你已被对方拉黑，不能发送文件" << std::endl;
            return;
        }
        std::string ID = res["ID"];
        fileclient_->sendfile(ID, filepath, filename, std::to_string(filesize),false);
}
void chatclient::handle_chatsendfile(std::string frienduser){
    std::cout << std::endl;
    for (int i = 0; i < 75; i++) {
        std::cout  << "-";
    }
    std::cout << std::endl;
   if(!inputfilepath()){
       return;
   }
    json j;
    j["cmd"] = "sendfile";
    j["from"] = account;
    j["ischat"] = "1";
    j["to"] = frienduser;
    j["filename"] = filename;
    j["filepath"] = filepath;
    j["filesize"] = std::to_string(filesize);
    id = gen_req_id();
    j["request_id"] = id;
    json res;
    if (!resfuture(id, j, res)) {
        return;
    }
    if(res["code"]=="1"){
        std::cout << PURPLE << "你已被对方拉黑，不能发送文件" << std::endl;
        return;
    }else{
        std::string ID = res["ID"];
        int bb = fileclient_->sendfile(ID, filepath, filename,
                                       std::to_string(filesize), true);
        for (int i = 0; i < 75; i++) {
            std::cout << "-";
        }
        std::cout << std::endl;
        json j1;
        j1["cmd"] = "friendchat";
        j1["from"] = account;
        j1["message"] = "[上传文件]:" + filename;
        j1["to"] = frienduser;
        if (isblocklist[chatfriendname]) {
            std::cout << PURPLE
                      << "你当前已经被用户拉黑，请"
                         "退出聊天"
                      << std::endl;
        } else {
            chat_conn->send(j1.dump() + '\n');
        }
    }
}
void chatclient::handle_groupsendfile(){
    printfgrouplist();
    std::string groupname;
    char* line = readline(PURPLE "请输入您要发送文件群聊名称:" GREEN);
    if (!inputvail(line)) {
        return;
    }
    groupname= line;
    free(line);
    bool b= is_existsgroup(groupname);
    if(!b){
        std::cout << PURPLE << "该群聊并不存在" << RESET << std::endl;
        return;
    }else if(!is_groupmember2(groupname)){
        std::cout << PURPLE << "你并不在该群聊里" << RESET << std::endl;
        return;
    }else{
        if (!inputfilepath()) {
            return;
        }
        json j;
        j["cmd"] = "groupsendfile";
        j["from"] = account;
        j["ischat"] = "0";
        j["groupname"] = groupname;
        j["filename"] = filename;
        j["filepath"] = filepath;
        j["filesize"] = std::to_string(filesize);
        id = gen_req_id();
        j["request_id"] = id;
        json res;
        if (!resfuture(id, j, res)) {
            return;
        }
        if(res["code"]=="1"){
            std::cout << PURPLE << "你并不在改群聊，不能上传文件" << std::endl;
            return;
        }
        std::string ID = res["ID"];
        fileclient_->groupsendfile(ID, filepath, filename,
                                   std::to_string(filesize), false);
    }
}
