#include "friend.h"
bool sendn(int fd, char* data, ssize_t len) {
    while (len > 0) {
        ssize_t n = send(fd, data, len, 0);
        if (n <= 0) {
            return false;
        }
        data += n;
        len -= static_cast<ssize_t>(n);
    }
    return true;
}
     Friend::Friend() { 
        redis_.connect("127.0.0.1", 6379);
        redis_.sync_commit();
     }
    bool Friend::addapply(std::string applyaccount,std::string appliedaccount){
        auto fut1 = redis_.exists({applyaccount});
        redis_.sync_commit();

        int num1 = fut1.get().as_integer();
        if(num1){
            redis_.sadd("apply" + applyaccount, {applyaccount});
            redis_.sadd("applied" + appliedaccount, {appliedaccount});
            redis_.sync_commit();
            std::cout << "好友申请发送成功!" << std::endl;
        } else {
            std::cout << "该用户不存在" << std::endl;
        }
        return true;
    }
    bool Friend::agreeapply(std::string applyaccount,std::string appliedaccount){
        redis_.sadd("friend" + applyaccount, {appliedaccount});
        redis_.sadd("friend" + appliedaccount, {applyaccount});
        redis_.srem("applied" + appliedaccount, {applyaccount});
        redis_.srem("apply" + applyaccount, {appliedaccount});
        redis_.sync_commit();
        std::cout << "好友添加成功" << std::endl;
        return true;
    }
    bool Friend::refuseapply(std::string applyaccount, std::string appliedaccount){
        redis_.srem("applied" + appliedaccount, {applyaccount});
        redis_.srem("apply" + applyaccount, {appliedaccount});
        redis_.sync_commit();
        return true;
        }
    bool Friend::block(std::string applyaccount, std::string appliedaccount){
        redis_.sadd("block" + applyaccount, {appliedaccount});
        redis_.sadd("blocked" + appliedaccount, {applyaccount});
        redis_.sync_commit();
        return true;
    }
    bool Friend::cancleblock(std::string applyaccount, std::string appliedaccount){
        redis_.srem("block" + applyaccount, {appliedaccount});
        redis_.srem("blocked" + appliedaccount, {applyaccount});
        redis_.sync_commit();
        return true;
    }
    bool Friend::delfriend(std::string applyaccount, std::string appliedaccount){
        redis_.srem("friend" + applyaccount, {appliedaccount});
        redis_.srem("friend" + appliedaccount, {applyaccount});
        redis_.sync_commit();
        std::cout << "好友删除成功!" << std::endl;
        return true;
    }
    bool Friend::isfriend(std::string applyaccount, std::string appliedaccount){
        auto fut = redis_.exists({"friend" + applyaccount});
        redis_.sync_commit();

        int num = fut.get().as_integer();
        return num == 1;
    }
    bool Friend::isblock(std::string applyaccount, std::string appliedaccount){
        auto fut = redis_.exists({"block" + applyaccount});
        redis_.sync_commit();

        int num = fut.get().as_integer();
        return num == 1;
    }
    bool Friend::isonline(std::string account){
        auto fut = redis_.exists({"online" + account});
        redis_.sync_commit();

        int num = fut.get().as_integer();
        return num == 1;
    }
 
    std::vector<std::string> Friend::friendlist(std::string account){
        std::vector<std::string> list;
        auto futs = redis_.smembers("friend" + account);
        redis_.sync_commit();
        auto reply = futs.get();
        std::cout << "您的好友列表:" << std::endl;
        for (auto fut : reply.as_array()) {
            list.push_back(fut.as_string());
        }
        std::cout << "\n";
        return list;
    }
    std::vector<std::string> Friend::onlienlist(std::string account){
        std::vector<std::string> list;
        auto futs = redis_.smembers("online" + account);
        redis_.sync_commit();

        for(auto fut:futs.get().as_array()){
            list.push_back(fut.as_string());
        }
        return list;
    }
int main(){
    Friend f;
     curl_global_init(CURL_GLOBAL_ALL);
    verifycode c;
    std::string server = "3541053286@qq.com";
    int clientfd;
    int choice;
    while (1) {
        std::cout << "账号管理";
        std::cout << "1. 注册\n";
        std::cout << "2. 密码登录\n";
        std::cout << "3. 验证码登录\n";
        std::cout << "4. 忘记密码（发送原密码到邮箱）\n";
        std::cout << "5. 注销账号\n";
        std::cout << "0. 退出\n";
        std::cout << "请选择: ";
        std::cin >> choice;

        switch (choice) {
            case 1:{
                c.signup();
                break;
            }
                
            case 2:{
                std::string account;
                std::cout << "请输入qq邮箱账号:";
                std::cin >> account;
                c.loginwithkey(server, clientfd, account);
                int num;
                bool sta = true;
                while (sta) {
                    std::cout << "好友管理\n";
                    std::cout << "1.申请添加好友\n";
                    std::cout << "2.申请删除好友\n";
                    std::cout << "3.申请查看好友在线状态\n";
                    std::cout << "4.拉黑好友\n";
                    std::cout << "5.查看好友列表\n";
                    std::cout << "0.跳回登录界面\n";
                    std::cin >> num;
                    switch (num) {
                        case 1:{
                            std::cout << "请输入要添加好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            f.addapply(account, applied);
                            f.agreeapply(account, applied);
                            break;
                        }
                            
                        case 2:{
                            std::cout << "请输入要删除好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            f.delfriend(account, applied);
                            break;
                        }
                            
                        case 3:{
                            std::cout << "请输入要查看在线状态好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            bool res = f.isonline(applied);
                            if (res) {
                                std::cout << "好友在线" << std::endl;
                            } else {
                                std::cout << "好友当前不在线" << std::endl;
                            }
                            break;
                        }
                            
                        case 4:{
                            std::cout << "请输入要删除好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            f.delfriend(account, applied);
                            std::cout << "删除好友成功!" << std::endl;
                            break;
                        }
                           
                        case 5:{
                            std::vector<std::string> friendres =
                                f.friendlist(account);
                            for (auto a : friendres) {
                                std::cout << a << std::endl;
                            }
                            std::cout << "\n";
                            break;
                        }

                        case 0:{
                            sta = false;
                            break;
                        }
                           
                    }
                }
                break;
            }
                
            case 3:{
               
                
                std::string account;
                std::cout << "请输入qq邮箱账号:";
                std::cin >> account;
                c.loginwithcode(server, clientfd,account);
                int num;
                bool sta = true;
                while (sta) {
                    std::cout << "好友管理\n";
                    std::cout << "1.申请添加好友\n";
                    std::cout << "2.申请删除好友\n";
                    std::cout << "3.申请查看好友在线状态\n";
                    std::cout << "4.拉黑好友\n";
                    std::cout << "5.查看好友列表\n";
                    std::cout << "0.跳回登录界面\n";
                    std::cin >> num;
                    switch (num) {
                        case 1: {
                            std::cout << "请输入要添加好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            f.addapply(account, applied);
                            break;
                        }

                        case 2: {
                            std::cout << "请输入要删除好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            f.delfriend(account, applied);
                            break;
                        }

                        case 3: {
                            std::cout << "请输入要查看在线状态好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            bool res = f.isonline(applied);
                            if (res) {
                                std::cout << "好友在线" << std::endl;
                            } else {
                                std::cout << "好友当前不在线" << std::endl;
                            }
                            break;
                        }

                        case 4: {
                            std::cout << "请输入要删除好友的账号:";
                            std::string applied;
                            std::cin >> applied;
                            f.delfriend(account, applied);
                            std::cout << "删除好友成功!" << std::endl;
                            break;
                        }

                        case 5: {
                            std::vector<std::string> friendres =
                                f.friendlist(account);
                            std::cout << "您的好友列表:" << std::endl;
                            for (auto a : friendres) {
                                
                                std::cout << a << std::endl;
                            }
                            break;
                        }

                        case 0: {
                            sta = false;
                            break;
                        }
                    }
                }
                break;
            }
                
            case 4:{
                c.forgetkey(server);
                break;
            }
               
            case 5:{
                c.destroy(server);
                break;
            }
               
            case 0:{
                std::cout << "再见" << std::endl;
                curl_global_cleanup();
                return 0;
            }
               
            default:
                std::cout << "选择错误" << std::endl;
        }
}
}