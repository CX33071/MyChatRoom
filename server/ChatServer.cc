#include "ChatServer.h"
ChatServer::ChatServer(EventLoop* loop, std::string name, const InetAddress& addr):server_(loop,name,addr),loop_(loop){
    server_.setThreadNum(4);
    verifycode.resetatstus();
    server_.setConnectionCallback(
        [this](const TcpConnectionPtr& conn) { connectioncallback(conn); });

    server_.setMessageCallback(
        [this](const TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
            messagecallback(conn, buf, t);
        });
    LOG_INFO << "服务器启动";
    loop_->runEvery(20, [this]() { checkoutclientconn(); });
    server_.start();
}
void ChatServer::connectioncallback(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO << "有一个chatclient客户端连接成功:" << conn->peerAddress().toIpPort();
        LOG(INFO) << "GLog:chatclient连接成功!";
    } else {
        LOG_INFO << "有一个chatclient客户端下线了:" << conn->peerAddress().toIpPort();
        std::lock_guard<std::mutex> lock(g_mutex);
        for (auto it = clientmap.begin(); it != clientmap.end(); ++it) {
            if (it->second == conn) {
                std::string account = it->first;
                LOG_INFO << "用户 " << account << " 断开连接，设置为离线";
                verifycode.exitlogin(account);
                clientmap.erase(it);
                break;
            }
        }
    }
}
void ChatServer::checkoutclientconn(){
    for(auto account:clientmap){
        Timestamp t = Timestamp::now();
        Timestamp t1 = clientconntime[account.first];
        double tt = T.timeDifference(t, t1);
        if(tt>35.0){
            LOG_INFO << "有一个chatclient客户端下线了:"
                     << account.second->peerAddress().toIpPort();
            std::lock_guard<std::mutex> lock(g_mutex);
            clientconntime.erase(account.first);
            for (auto it = clientmap.begin(); it != clientmap.end(); ++it) {
                if (it->second == account.second) {
                    std::string account = it->first;
                    LOG_INFO << "用户 " << account << " 断开连接，设置为离线";
                    verifycode.exitlogin(account);
                    clientmap.erase(it);
                    break;
                }
            }
        }
        }
    }
std::string ChatServer::get_current_time() {
    Timestamp now = Timestamp::now();
    return now.toFormattedString(true);
}
void ChatServer::messagecallback(const TcpConnectionPtr& conn,
                                 Buffer* buf,
                                 Timestamp) {
    while (1) {
        const char* pos = buf->findn();
        if (!pos)
            break;
        std::string msg(buf->peek(), pos - buf->peek());
        buf->retrieveUntil(pos + 1);

        json j;
        j = json::parse(msg);
        std::string cmd = j["cmd"];
        if(!cmd.empty()){
            std::string account;
            for (auto a : clientmap) {
                if(a.second==conn){
                    account == a.first;
                }
            }
            clientconntime[account] = Timestamp::now();
        }
        std::cout << "收到命令cmd=" << cmd << std::endl;
        if(cmd=="getaccount"){
            std::string name=j["name"];
            std::string account=verifycode.getaccount(name);
            json j1;
            j1["data"] = account;
            std::string request_id = j["request_id"];
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if(cmd=="getname"){
            std::string account = j["account"];
            std::string name=verifycode.getname(account);
            json j1;
            j1["data"]=name;
            std::string request_id = j["request_id"];
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if(cmd=="isexistsname"){
            std::string name=j["name"];
            json j1;
            if (verifycode.isexistsname(name)) {
                j1["code"] = "1";
            }else{
                j1["code"] = "0";
            }
            std::string request_id = j["request_id"];
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if(cmd=="is_online"){
            std::string account=j["account"];
            json j1;
            j1["cmd"] = "is_onlineres";
            bool res = verifycode.is_online(account);
            if(res){
                j1["code"] = "1";
            }else{
                j1["code"] = "2";
            }
              std::string request_id = j["request_id"];
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "signup") {
            std::string account = j["account"];
            std::string password = j["password"];
            std::string name = j["name"];
            std::cout << "name =" << name << std::endl;
            bool res = verifycode.signup(account, password, name);
            json j1;
            j1["cmd"] = "signup_res";
            if (res) {
                j1["data"] = "注册成功!";
            } else {
                j1["data"] = "该账号已被注册过，请重新尝试";
            }
            std::string request_id = j["request_id"];
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "verifycode") {
            json j1;
            std::string account = j["account"];
            bool ret = verifycode.addredis(account);
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            j1["cmd"] = "verifycode_res";
            if (ret) {
                j1["data"] = "验证码成功发送!";
                j1["code"] = "1";
            } else {
                j1["data"] = "帐号不存在!";
                j1["code"] = "0";
            }
            conn->send(j1.dump() + '\n');
            std::cout << "123" << std::endl;
        }
        if (cmd == "verifycodesignin") {
            std::string account = j["account"];
            std::string code = j["code"];
            bool res = verifycode.verify(account, code);
            json j1;
            j1["cmd"] = "codesignin_res";
            if (res) {
                j1["code"] = "1";
                j1["data"] = "验证码正确，登录成功";
                std::lock_guard<std::mutex> lock(g_mutex);
                clientmap[account] = conn;
                std::string request_id = j.value("request_id", "");
                if (!request_id.empty()) {
                    j1["request_id"] = request_id;
                }
                conn->send(j1.dump() + '\n');
                std::vector<std::string> disconnectmsg =
                    G.getdisconnectmsg(account);
                G.destorydismsg(account);
                for (auto it : disconnectmsg) {
                    json jmsg = json::parse(it);
                    conn->send(jmsg.dump() + '\n');
                }
            } else {
                j1["code"] = "2";
                j1["data"] = "验证码错误，登录失败";
                std::string request_id = j.value("request_id", "");
                if (!request_id.empty()) {
                    j1["request_id"] = request_id;
                }
                conn->send(j1.dump() + '\n');
            }
        }
        if (cmd == "keysignin") {
            std::string account = j["account"];
            std::string password = j["password"];
            int res = verifycode.loginwithkey(account, password);
            json j1;
            j1["cmd"] = "keysignin_res";
            if (res == 0) {
                j1["code"] = "1";
                j1["data"] = "密码正确，登录成功";
                std::lock_guard<std::mutex> lock(g_mutex);
                clientmap[account] = conn;
                std::string request_id = j.value("request_id", "");
                if (!request_id.empty()) {
                    j1["request_id"] = request_id;
                } else {
                }
                conn->send(j1.dump() + '\n');
                std::vector<std::string> disconnectmsg =
                    G.getdisconnectmsg(account);
                G.destorydismsg(account);
                for (auto it : disconnectmsg) {
                    json jmsg = json::parse(it);
                    conn->send(jmsg.dump() + '\n');
                }
            } else if (res == 2) {
                j1["code"] = "2";
                j1["data"] = "密码错误，登录失败";
                std::string request_id = j.value("request_id", "");
                if (!request_id.empty()) {
                    j1["request_id"] = request_id;
                } else {
                }
                conn->send(j1.dump() + '\n');

            } else {
                j1["code"] = "2";
                j1["data"] = "该账号并不存在";
                std::string request_id = j.value("request_id", "");
                if (!request_id.empty()) {
                    j1["request_id"] = request_id;
                } else {
                }
                conn->send(j1.dump() + '\n');
            }
        }
        if (cmd == "exitlogin") {
            std::string account = j["account"];
            std::cout << "进入verify函数" << std::endl;
            verifycode.exitlogin(account);
            json reply;
            reply["cmd"] = "exitloginres";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                reply["request_id"] = request_id;
            }
            reply["data"] = "[系统消息]:已经退出登录";
            conn->send(reply.dump() + '\n');
            std::cout << "exitlogin已回复" << std::endl;
        }
        if (cmd == "forgetkey") {
            std::string account = j["account"];
            bool res = verifycode.forgetkey(account);
            json j1;
            j1["cmd"] = "forgetkey_res";
            if (res) {
                j1["data"] = "密码已经发到您的邮箱";
            } else {
                j1["data"] = "\n该账号并未注册";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "sendfile") {
            std::string from = j["from"];
            std::string to = j["to"];
            std::string filename = j["filename"];
            std::string filesize = j["filesize"];
            fileid++;
            std::string docu_id = std::to_string(fileid);
            file.begin(from, to, filename, filesize, docu_id);
            json res;
            res["cmd"] = "sendfileres";
            res["ID"] = docu_id;
            res["data"] = "已发送上传文件请求给服务端";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                res["request_id"] = request_id;
            }
            conn->send(res.dump() + '\n');
        }
        if(cmd=="recvfile"){
            std::string from=j["from"];
            std::string filename = j["filename"];
            std::string ID = j["ID"];
            json j1;
            j1["cmd"]="recvfileres";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            std::string filesize = file.getfilesize(ID);
            j1["filesize"] = filesize;
            conn->send(j1.dump() + '\n');
        }
        if(cmd=="RETR_ok"){
            std::string ID = j["ID"];
            std::string filename = j["filename"];
            std::string from = file.getfrom(ID);
            std::string account = j["account"];
            json j2, j1;
            // j1["cmd"] = "finish";
            // j1["data"] = "下载成功";
            // std::string request_id = j.value("request_id", "");
            // if (!request_id.empty()) {
            //     j1["request_id1"] = request_id;
            // }
            // conn->send(j1.dump() + '\n');
            j2["cmd"] = "toRETRres";
            std::string to = file.finish(ID);
            file.setloadfinish(ID);
            std::string name = verifycode.getname(account);
            j2["data"] = "用户" + name + "下载了您发送的文件" + filename;
            j2["time"] = get_current_time();
            TcpConnectionPtr target_conn;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = clientmap.find(from);
                if (it != clientmap.end()) {
                    target_conn = it->second;
                } else {
                    G.disconnectmsg(to, j2);
                }
            }
            if (target_conn) {
                target_conn->send(j2.dump() + '\n');
            }
        }
        if(cmd=="sendfile_finish"){
            std::string ID = j["ID"];
            std::string filename=j["filename"];
            std::string to=file.finish(ID);
            std::string from = file.getfrom(ID);
            std::cout << "to =" << to << std::endl;
            json j2;
            j2["cmd"]="sendedfile";
            std::string name = verifycode.getname(from);
            std::string t = get_current_time();
            j2["time"] = t;
            j2["data"] = "您收到用户" + name + "发送的文件:" + filename+"   文件ID:"+ID+t;
            j2["from"]=from;
            j2["filename"] = filename;
            j2["ID"] = ID;
            TcpConnectionPtr target_conn;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = clientmap.find(to);
                if (it != clientmap.end()) {
                    target_conn = it->second;
                } else {
                    G.disconnectmsg(to, j2);
                }
            }
            if (target_conn) {
                target_conn->send(j2.dump() + '\n');
                std::cout << "给目标用户发送接收文件消息" << std::endl;
            }else{
                std::cout << "目标用户当前不在线" << std::endl;
            }
        }
        if (cmd == "groupsendfile_finish") {
            std::string ID = j["ID"];
            std::string filename = j["filename"];
            std::string groupname1 = file.finish(ID);
            std::string from = file.getfrom(ID);
            json j2;
            j2["cmd"] = "groupsendedfile";
            std::string t = get_current_time();
            j2["time"] = t;
            std::string name = verifycode.getname(from);
            j2["data"] = "您收到来自群聊" + groupname1 + "的用户" + name +
                         "发送的文件:" + filename + "   文件ID:" + ID + t;
            j2["from"] = from;
            j2["filename"] = filename;
            j2["ID"] = ID;
            std::vector<std::string> res =
                G.grouptargetmember(groupname1, from);
            for (int i = 0; i < res.size(); i++) {
                std::string target = res[i];
                std::cout << "target = " << target << std::endl;
                TcpConnectionPtr target_conn;
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    auto it = clientmap.find(target);
                    if (it != clientmap.end()) {
                        target_conn = it->second;
                    } else {
                        G.disconnectmsg(target, j2);
                    }
                }
                if (target_conn) {
                    target_conn->send(j2.dump() + '\n');
                }
            }
        }
        if (cmd == "destory") {
            std::string account = j["account"];
            std::string password = j["password"];
            int res = verifycode.destroy(account, password, G, F);
            json j1;
            j1["cmd"] = "destory_res";
            if (res == 1) {
                j1["data"] = "\n该账号并不存在";
            } else if (res == 2) {
                j1["data"] = "\n密码错误，注销失败";
            } else {
                j1["data"] = "\n密码正确，成功注销!";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "addfriend") {
            std::string from = j["from"];
            std::string to = j["to"];
            bool res = F.addapply(from, to);
            json j1;
            j1["cmd"] = "addres";
            if (res) {
                j1["data"] = "好友申请已经发送";
                json j2;
                std::string name = verifycode.getname(from);
                std::string s = "有一条来自" + name + "的好友申请";
                j2["cmd"] = "addedres";
                j2["message"] = s;
                j2["target"] = from;
                std::string t = get_current_time();
                j2["time"] = t;
                TcpConnectionPtr target_conn;
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    auto it = clientmap.find(to);
                    if (it != clientmap.end()) {
                        target_conn = it->second;
                    } else {
                        G.disconnectmsg(to, j2);
                    }
                }
                if (target_conn) {
                    target_conn->send(j2.dump() + '\n');
                }

            } else {
                j1["data"] = "要添加的好友账号不存在";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "delmember") {
            std::string groupname = j["groupname"];
            std::string account = j["account"];
            G.delmember(groupname, account);
            json j1;
            j1["cmd"] = "delmemberres";
            j1["data"] = "移出群成员成功!";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "applyjoingroup") {
            std::string groupname = j["groupname"];
            std::string account = j["account"];
            std::vector<std::string> res = G.applyjoingroup(account, groupname);
            json j1, j2;
            j1["cmd"] = "applyjoingroupres";
            j1["data"] = "申请入群消息已经发送";
            std::string name = verifycode.getname(account);
            std::string s =
                "有一条来自" + name + "加入群:" + groupname + "的申请";
            j2["cmd"] = "appliedjoinres";
            j2["data"] = s;
            j2["groupname"] = groupname;
            j2["account"] = account;
            std::string t = get_current_time();
            j2["time"] = t;
            for (auto i = 0; i < res.size(); i++) {
                std::string target = res[i];
                TcpConnectionPtr target_conn;
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    auto it = clientmap.find(target);
                    if (it != clientmap.end()) {
                        target_conn = it->second;
                        std::cout << "对方在线" << std::endl;
                    } else {
                        std::cout << "对方不在线" << std::endl;
                        G.disconnectmsg(target, j2);
                    }
                }
                if (target_conn) {
                    target_conn->send(j2.dump() + '\n');
                }
            }

            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "getgrouphistory") {
            std::string groupname = j["groupname"];
            std::vector<std::string> res = G.getgrouphistory(groupname);
            json j1;
            j1["cmd"] = "getgrouphistoryres";
            std::string his;
            for (int i = 0; i < res.size(); i++) {
                his += res[i];
                his += "\n";
            }
            j1["data"] = his;
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "agreefriend") {
            std::string account = j["account"];
            std::string friendaccount = j["friendaccount"];
            std::cout << "friendaccount=" << friendaccount << std::endl;
            F.agreeapply(account, friendaccount);
            json j1, j2;
            j1["cmd"] = "agreeres";
            j2["cmd"] = "agreedres";
            j1["data"] = "同意对方的好友申请";
            std::string name = verifycode.getname(account);
            j2["data"] =
                name + "已经同意你的好友申请" + "\n" + "请继续菜单输入:";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
            TcpConnectionPtr target_conn;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = clientmap.find(friendaccount);
                if (it != clientmap.end()) {
                    target_conn = it->second;
                    std::cout << "pppp当前在线" << std::endl;
                } else {
                    std::cout << "用户当前不在线" << std::endl;
                    G.disconnectmsg(friendaccount, j2);
                }
            }
            if (target_conn) {
                target_conn->send(j2.dump() + '\n');
            }
        }
        if (cmd == "refusefriend") {
            std::string account = j["account"];
            std::string friendaccount = j["friendaccount"];
            F.refuseapply(account, friendaccount);
            json j1, j2;
            std::string name = verifycode.getname(account);
            j1["cmd"] = "refuseres";
            j2["cmd"] = "refusedres";
            j1["data"] = "已经拒绝对方的好友申请";
            j2["data"] =
                name + "拒绝了你的好友申请" + "\n" + "请继续菜单输入:";
            TcpConnectionPtr target_conn;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = clientmap.find(friendaccount);
                if (it != clientmap.end()) {
                    target_conn = it->second;
                } else {
                    G.disconnectmsg(friendaccount, j2);
                }
            }
            if (target_conn) {
                target_conn->send(j2.dump() + '\n');
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if(cmd=="groupsendfile"){
            std::string from=j["from"];
            std::string groupname=j["groupname"];
            std::string filename = j["filename"];
            std::string filesize = j["filesize"];
            fileid++;
            std::string docu_id = std::to_string(fileid);
            file.begin(from, groupname, filename, filesize, docu_id);
            json res;
            res["cmd"] = "sendfileres";
            res["ID"] = docu_id;
            res["data"] = "已发送上传文件请求给服务端";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                res["request_id"] = request_id;
            }
            conn->send(res.dump() + '\n');
        }
        if (cmd == "friendlist") {
            std::string account = j["account"];
            std::vector<std::string> res = F.friendlist(account);
            std::string list = "好友列表:\n";
            if(res.size()==0){
                list += "无";
                list += "\n";
            }
            for (int i = 0; i < res.size(); i++) {
                std::string name = verifycode.getname(res[i]);
                std::cout << "name =" << name << std::endl;
                ;
                list += name;
                list += "\n";
            }
            json j1;
            j1["cmd"] = "friendlistres";
            j1["data"] = list;
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }

        if (cmd == "groupmember") {
            std::string groupname = j["groupname"];
            std::string account = j["account"];
            std::vector<std::string> res = G.groupmembers(account, groupname);
            json j1;
            j1["cmd"] = "groupnameres";
            if (res.empty()) {
                j1["data"] = "您并不在该群聊当中，无查看权限";
            } else if (res[0] == "NULL") {
                j1["data"] = "该群聊并不存在";
            } else {
                std::string list = "群成员列表:\n";
                for (int i = 0; i < res.size(); i++) {
                    list += res[i];
                    list += "\n";
                }

                j1["data"] = list;
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
            
        }
        if (cmd == "is_groupmember") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            int res = G.is_groupmember(groupname, account);
            json j1;
            j1["cmd"] = "is_groupmemberres";
            if (res) {
                j1["code"] = "1";
            } else {
                j1["code"] = "2";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "is_existsgroup") {
            std::string groupname = j["groupname"];
            int res = G.is_existsgroup(groupname);
            json j1;
            j1["cmd"] = "is_existsgroupres";
            if (res) {
                j1["code"] = "1";
            } else {
                j1["code"] = "2";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "is_manager") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            int res = G.is_manager(groupname, account);
            json j1;
            j1["cmd"] = "is_managerres";
            if (res) {
                j1["code"] = "1";
            } else {
                j1["code"] = "2";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "addmanager") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            G.addmanager(groupname, account);
            json j1;
            j1["cmd"] = "addmanagerres";
            j1["data"] = "添加管理员成功!";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "delmanager") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            G.delmanager(groupname, account);
            json j1;
            j1["cmd"] = "delmanagerres";
            j1["data"] = "删除管理员成功!";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "grouplist") {
            std::string account = j["account"];
            std::vector<std::string> res = G.grouplist(account);
            std::string list = "加入的群聊列表:\n";
            for (int i = 0; i < res.size(); i++) {
                list += res[i];
                list += "\n";
            }
            json j1;
            j1["cmd"] = "grouplistres";
            j1["data"] = list;
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if(cmd=="ownergrouplist"){
            std::string account = j["account"];
            std::vector<std::string> res = G.ownergrouplist(account);
            std::string list = "你创建的群聊:\n";
            for (int i = 0; i < res.size(); i++) {
                list += res[i];
                list += "\n";
            }
            json j1;
            j1["cmd"] = "ownergrouplistres";
            j1["data"] = list;
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "blocklist") {
            std::string account = j["account"];
            std::vector<std::string> res = F.blocklist(account);
            std::string list = "拉黑好友列表:\n";
            if(res.size()==0){
                list += "无";
                list += "\n";
            }
            for (int i = 0; i < res.size(); i++) {
                std::string name=verifycode.getname(res[i]);
                list += name;
                list += "\n";
            }
            json j1;
            j1["cmd"] = "blocklistres";
            j1["data"] = list;
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "onlinelist") {
            std::string account = j["account"];
            std::vector<std::string> res = F.onlinelist(account);
            std::string list = "好友在线情况:\n";
            if(res.size()==0){
                list+="当前无好友";
                list += "\n";
            }
            for (int i = 0; i < res.size(); i++) {
                list += res[i];
                list += "\n";
            }
            json j1;
            j1["cmd"] = "onlinelistres";
            j1["data"] = list;
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "is_friend") {
            json j1;
            j1["cmd"] = "is_friendres";
            std::string account = j["account"];
            std::string friendaccount = j["friendaccount"];
            int res = F.isfriend(account, friendaccount);
            j1["data"] = "";
            std::vector<std::string> historymsg =
                F.gethistoryfriendchat(account, friendaccount);
            std::string his;
            for (int i = 0; i < historymsg.size(); i++) {
                his += historymsg[i];
                his += "\n";
                j1["data"] = his;
            }
            if (res == 1) {
                j1["code"] = "1";
            } else if (res == 2) {
                j1["code"] = "2";
            } else {
                j1["code"] = "0";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "friendchat") {
            std::cout << "收到好友发来消息" << std::endl;
            std::string target = j["to"];
            std::string account = j["from"];
            std::string msg = j["message"];
            std::cout << "msg.size()=" << msg.size() << std::endl;
            json j2;
            j2["cmd"] = "chatedres";
            j2["account"] = account;
            j2["message"] = msg;
            std::string t = get_current_time();
            j2["time"] = t;
            std::string name1 = verifycode.getname(account);
            F.historyfriendchat(
                account, target,
                "[" + name1 + "]:  " + msg + "         [" + t + "]");
            TcpConnectionPtr target_conn;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = clientmap.find(target);
                if (it != clientmap.end()) {
                    target_conn = it->second;
                    target_conn->send(j2.dump() + '\n');
                    std::cout << "已经给目标用户发过去" << std::endl;
                } else {
                    G.disconnectmsg(target, j2);
                }
            }
        }
        if (cmd == "block") {
            std::string account = j["account"];
            std::string target = j["target"];
            bool res = F.block(account, target);
            F.delfriend1(account, target);
            json j1;
            j1["cmd"] = "blockres";
            if (res) {
                j1["data"] = "已经拉黑" + target;
            } else {
                j1["data"] = "拉黑失败，该用户不存在";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "is_block") {
            json j1;
            std::string account = j["account"];
            std::string friendaccount = j["friendaccount"];
            int res = F.isblock(account, friendaccount);
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            if (res == 1) {
                j1["data"] = "1";
            } else if (res == 2) {
                j1["data"] = "2";
            } else {
                j1["data"] = "0";
            }
            conn->send(j1.dump() + '\n');
        }
        if(cmd=="isexists"){
            std::string account=j["account"];
            std::cout << "account = " << account << std::endl;
            int res = verifycode.isexists(account);
            std::cout << "res = " << res << std::endl;
            json j1;
            j1["cmd"] = "isexistsres";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            if (res == 1) {
                j1["code"] = "1";
            }else{
                j1["code"] = "2";
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "cancleblock") {
            std::string account = j["account"];
            std::string target = j["target"];
            int res = F.cancleblock(account, target);
            F.addfriend(account, target);
            json j1;
            j1["cmd"] = "cancleres";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            j1["data"] = "已成功取消拉黑";
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "delfriend") {
            std::string account = j["account"];
            std::string target = j["target"];
            int res = F.delfriend(account, target);
            std::cout << "account =" << account << "    target =" << target
                      << "  res =" << res << std::endl;
            json j1;
            j1["cmd"] = "delfriendres";
            if (res == 1) {
                j1["data"] = "目标用户不存在";
            } else if (res == 2) {
                j1["data"] = "目标用户还不是好友";
            } else {
                j1["data"] = "已删除目标用户好友";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "creategroup") {
            std::string groupname = j["groupname"];
            std::string account = j["account"];
            std::string res = G.creategroup(account, groupname);
            json j1;
            j1["cmd"] = "creategroupres";
            j1["data"] = "群聊已成功创建:" + res;
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "exitgroup") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            int res = G.exitgroup(account, groupname);
            json j1;
            j1["cmd"] = "exitgroupres";
            if (res == 1) {
                j1["data"] = "您并不在该群聊当中";
            } else if (res == 2) {
                j1["data"] = "该群聊并不存在";
            } else {
                j1["data"] = "成功退出该群聊";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "agreejoingroup") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            std::cout << "进入agreejoin" << std::endl;
            G.agreejoin(account, groupname);
            std::cout << "完成agreejoin函数" << std::endl;
            json j1, j2;
            j1["cmd"] = "agreejoingroupres";
            j1["data"] = "您已同意该用户的入群申请";
            j2["cmd"] = "agreedjoingroupres";
            j2["data"] =
                "您已经通过群聊[" + groupname + "]管理员验证，成功进入该群聊";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
            TcpConnectionPtr target_conn;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = clientmap.find(account);
                if (it != clientmap.end()) {
                    target_conn = it->second;
                } else {
                    G.disconnectmsg(account, j2);
                }
            }
            if (target_conn) {
                target_conn->send(j2.dump() + '\n');
            }
        }
        if (cmd == "refusejoingroup") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            G.refusejoin(account, groupname);
            json j1, j2;
            j1["cmd"] = "refusegroupres";
            j2["cmd"] = "refusedgroupres";
            j1["data"] = "已拒绝对方的入群邀请";
            j2["data"] = "群聊管理员拒绝了你的邀请入群申请";
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
            TcpConnectionPtr target_conn;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto it = clientmap.find(account);
                if (it != clientmap.end()) {
                    target_conn = it->second;
                } else {
                    G.disconnectmsg(account, j2);
                }
            }
            if (target_conn) {
                target_conn->send(j2.dump() + '\n');
            }
        }
        if (cmd == "delgroup") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            int res = G.delgroup(groupname, account);
            json j1;
            j1["cmd"] = "delgroupres";
            if (res == 0) {
                j1["data"] = "删除群聊成功";
            } else if (res == 1) {
                j1["data"] = "该群聊并不存在";
            } else if (res == 2) {
                j1["data"] = "你不是该群群主，没有解散群聊的权限!";
            } else if (res == 3) {
                j1["data"] = "密码错误，解散群聊失败";
            }
            std::string request_id = j.value("request_id", "");
            if (!request_id.empty()) {
                j1["request_id"] = request_id;
            }
            conn->send(j1.dump() + '\n');
        }
        if (cmd == "groupchat") {
            std::string account = j["account"];
            std::string groupname = j["groupname"];
            std::string message = j["message"];
            json j1;
            j1["cmd"] = "groupchatedres";
            j1["account"] = account;
            j1["message"] = message;
            j1["groupname"] = groupname;
            std::string t = get_current_time();
            j1["time"] = t;
            std::cout << "时间" << t << std::endl;
            std::string name = verifycode.getname(account);
            G.historygroupchat(
                account, groupname,
                "[" + name + "]:  " + message + "         [" + t + "]");
            std::vector<std::string> res =
                G.grouptargetmember(groupname, account);
            for (int i = 0; i < res.size(); i++) {
                std::string target = res[i];
                TcpConnectionPtr target_conn;
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    auto it = clientmap.find(target);
                    if (it != clientmap.end()) {
                        target_conn = it->second;
                    } else {
                        G.disconnectmsg(target, j1);
                    }
                }
                if (target_conn) {
                    target_conn->send(j1.dump() + '\n');
                }
            }
        }
    }
}