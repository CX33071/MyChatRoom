#include "/home/cx33071/muduo-/net/TcpClient.h"
#include <condition_variable>
#include <termios.h>
TcpClient::TcpConnectionPtr g_conn;
std::condition_variable g_cv;
std::mutex g_mutex;
bool connok = false;
bool is_login = false;
std::string account;
std::string cinkey() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::string key;
    std::cin >> key;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return key;
}
void connectioncallback(const TcpClient::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO << "成功连接服务器";
        g_conn = conn;
        connok = true;
        g_cv.notify_all();
    } else {
        LOG_INFO << "与服务器断开连接";
        g_conn.reset();
        connok = false;
    }
}
void messagecallback(const TcpClient::TcpConnectionPtr&conn,Buffer*buf,Timestamp){
    std::string message = buf->retrieveAllAsString();
    std::cout << "收到消息:" << message << std::endl;
}
void main_menu(){
    std::cout << "欢迎使用MyChatRoom!" << std::endl;
    std::cout << "    用户管理\n";
    std::cout << "1.注册\n";
    std::cout << "2.验证码登录\n";
    std::cout << "3.密码登录\n";
    std::cout << "4.忘记密码\n";
    std::cout << "5.注销账号\n";
    std::cout << "0.退出\n";
    std::cout << "请选择:";
}
void friend_menu(){
    std::cout << "    好友管理\n";
    std::cout << "1.添加好友\n";
    std::cout << "2.好友列表\n";
    std::cout << "3.私聊\n";
    std::cout << "4.拉黑好友\n";
    std::cout << "5.删除好友\n";
    std::cout << "0. 返回主菜单\n";
    std::cout << "请选择:";
}
void friendfunction(){
    while(is_login){
        friend_menu();
        int num;
        std::cin >> num;
        std::string frienduser;
        std::string servermsg;
        std::string chatmsg;
        switch (num) {
            case 1:
                std::cout << "请输入要添加好友的账号:";
                std::getline(std::cin, frienduser);
                g_conn->send("addfriend:" + account + " " + frienduser);
                break;
            case 2:
                g_conn->send("friendlist:" + account);
                break;
            case 3:
                std::cout << "请输入要私聊的好友账号:";
                std::cin >> frienduser;
                std::cout << "请输入要发送的信息:";
                std::getline(std::cin, chatmsg);
                g_conn->send("chat:" + account + " " + frienduser + ":" +
                             chatmsg);
                break;
            case 4:
                std::cout << "请输入要拉黑的好友账号:";
                std::cin >> frienduser;
                g_conn->send("block:" + account + " " + frienduser);
                break;
            case 5:
                std::cout << "请输入要删除的好友账号:";
                std::cin >> frienduser;
                g_conn->send("delfriend:" + account + " " + frienduser);
                break;
            case 0:
                is_login = false;
                break;
            default:
        }
    }
}
void mainfunction(){
    while(1){
        main_menu();
        int choice;
        std::cin >> choice;
        std::string password;
        std::string verifycode;
        std::string servermsg; 
        switch (choice) {
            case 1:
                std::cout << "请输入你的qq邮箱:";
                std::cin >> account;
                std::cout<<"请输入你的密码:";
                password = cinkey();
                g_conn->send("signup:" + account + " " + password);
            case 2:
                g_conn->send("verifycode:" + account);
                std::cout << "验证码已经发送到您的qq邮箱\n";
                std::cout << "请输入验证码:";
                std::cin >> verifycode;
                g_conn->send("verifycodesignin:" + account + verifycode);
                friendfunction();
            case 3:
                std::cout << "请输入您的密码:";
                password = cinkey();
                g_conn->send("keysignin:" + account + password);
                friendfunction();
            case 4:
                g_conn->send("forgetkey:" + account);
            case 5:
                std::cout << "请输入您的密码:";
                std::getline(std::cin, password);
                g_conn->send("destory:" + account+" "+password);
            case 0:
                exit(0);
            default:
                std::cout << "请输入有效选项!" << std::endl;
        };
    }
}
int main(int argc,char*argv[]){
    Logger logger;
    logger.setLogLevel(Logger::INFO);
    EventLoop loop;
    InetAddress addr(argv[1], 8888);
    TcpClient client(&loop, addr);
    client.setConnectionCallback(connectioncallback);
    client.setMessageCallback(messagecallback);
    client.connect();
    std::thread t(mainfunction);
    t.detach();
    int timeout;
    loop.loop(timeout);
    return 0;
}