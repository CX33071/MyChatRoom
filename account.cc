#include "account.h"
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
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* data = (std::string*)userp;
    size_t len = data->size();
    memcpy(ptr, data->c_str(), len);
    data->clear();
    return len;
}
verifycode::verifycode() {
    srand(time(NULL));
    redis_.connect("127.0.0.1", 6379);
}
std::string verifycode::generatesalt() {
    std::string salt;
    for (int i = 0; i < 10; i++) {
        salt += ('0' + rand() % 10);
    }
    return salt;
}
std::string verifycode::sha(std::string input) {
    std::reverse(input.begin(), input.end());
    return input;
}
std::string verifycode::screctkey(std::string key) {
    std::string salt = generatesalt();
    std::string hash = sha(salt + key);
    return salt + ":" + hash;
}
std::string verifycode::getstartkey(std::string hashkey) {
    size_t pos = hashkey.find(':');
    std::string salt = hashkey.substr(0, pos);
    std::string hash = hashkey.substr(pos + 1);
    std::reverse(hash.begin(), hash.end());
    std::string startkey = hash.substr(10);
    return startkey;
}
bool verifycode::checkkey(const std::string& inputkey,
                          const std::string& getkeyvalue) {
    size_t pos = getkeyvalue.find(':');
    std::string salt = getkeyvalue.substr(0, pos);
    std::string hash = getkeyvalue.substr(pos + 1);
    std::string inputhash = sha(salt + inputkey);
    return inputhash == hash;
}
std::string verifycode::code() {
    std::string code;
    for (int i = 0; i < 4; i++) {
        code += ('0' + rand() % 10);
    }
    return code;
}
void verifycode::addredis(const std::string& server,
                          const std::string& account) {
    std::string s = code();
    redis_.setex(account + "1", 300, s);
    redis_.sync_commit();
    sendcom(server, account, "验证码", "您的验证码是: " + s + " 5分钟内有效");
}
void verifycode::signup() {
    std::string account, key;
    std::cout << "请输入您的qq邮箱:";
    std::cin >> account;
    auto fut = redis_.exists({account});
    redis_.sync_commit();
    int exists = fut.get().as_integer();
    while (exists) {
        std::cout << "该账号已被使用\n请重新输入帐号: ";
        std::cin >> account;
        auto fut = redis_.exists({account});
        redis_.sync_commit();
        exists = fut.get().as_integer();
    }
    std::cout << "请设置您的密码:";
    key = cinkey();
    std::cout << std::endl;
    std::cout << "请再次输入密码:";
    std::string key1 = cinkey();
    std::cout << std::endl;
    while (key != key1) {
        std::cout << "两次密码设置不一致,请再次设置您的密码:";
        key = cinkey();
        std::cout << std::endl;
        std::cout << "请再次输入密码:";
        key1 = cinkey();
        std::cout << std::endl;
    }
    std::string finalkey = screctkey(key);
    redis_.set(account, finalkey);
    redis_.sync_commit();
    std::cout << "注册成功！";
}
bool verifycode::verify(const std::string& account,
                        const std::string& inputcode) {
    auto fut = redis_.get(account + "1");
    redis_.sync_commit();
    if (fut.get().as_string() == inputcode) {
        redis_.del({account + "1"});
        redis_.sync_commit();
        return true;
    }
    return false;
}
void verifycode::loginwithkey(const std::string& server, int fd,std::string account) {
    std::string  key;
    auto fut = redis_.get(account);
    redis_.sync_commit();
    auto reply = fut.get();
    if (!reply.is_string()) {
        std::cout << "该账号不存在，请先注册" << std::endl;
        exit(1);
    }
    std::cout << "请输入密码:";
    key = cinkey();
    std::cout << std::endl;
    std::string hashkey = reply.as_string();
    while (!checkkey(key, hashkey)) {
        std::cout << "密码错误，请选择 1重新输入密码 2为忘记密码: ";
        int option;
        std::cin >> option;
        if (option == 1) {
            std::cout << "请重新输入密码: ";
            key = cinkey();
            std::cout << std::endl;
        } else {
            forgetkey(server);
            std::cout << "请重新输入密码: ";
            key = cinkey();
            std::cout << std::endl;
        }
    }
    std::cout << "登录成功" << std::endl;
    redis_.set("online" + account, "1");
}
void verifycode::loginwithcode(const std::string& server, int fd,std::string account) {
    auto fut = redis_.exists({account});
    redis_.sync_commit();
    if (fut.get().as_integer() == 0) {
        std::cout << "该账号不存在，请先注册" << std::endl;
        return;
    }
    addredis(server, account);
    std::cout << "请输入验证码:";
    std::string code;
    std::cin >> code;
    while (!verify(account, code)) {
        redis_.del({account + "1"});
        redis_.sync_commit();
        std::cout << "验证码错误，新的验证码已发送，请重新输入: ";
        std::cin >> code;
    }
    std::cout << "登录成功" << std::endl;
    redis_.set("online" + account, "1");
}
void verifycode::forgetkey(const std::string& server) {
    std::string account;
    std::cout << "请输入您的qq邮箱账号:";
    std::cin >> account;
    auto fut = redis_.exists({account});
    redis_.sync_commit();
    if (fut.get().as_integer() == 0) {
        std::cout << "该账号并未注册" << std::endl;
        return;
    }
    addredis(server, account);
    std::cout << "请输入收到的验证码: ";
    std::string code;
    std::cin >> code;
    while (!verify(account, code)) {
        redis_.del({account + "1"});
        redis_.sync_commit();
        std::cout << "验证码错误，新的验证码已发送，请重新输入: ";
        std::cin >> code;
    }
    std::cout << "验证码正确" << std::endl;
    auto fut1 = redis_.get(account);
    redis_.sync_commit();
    std::string hashcode = fut1.get().as_string();
    std::string truecode = getstartkey(hashcode);
    sendcom(server, account, "密码", "您的密码是: " + truecode);
    std::cout << "密码已经发到您的邮箱" << std::endl;
}
void verifycode::destroy(const std::string& server) {
    std::string account, key;
    std::cout << "请输入您的账号:";
    std::cin >> account;
    auto fut = redis_.get(account);
    redis_.sync_commit();
    auto reply = fut.get();
    if (!reply.is_string()) {
        std::cout << "该账号不存在" << std::endl;
        return;
    }
    std::cout << "请输入密码:";
    key = cinkey();
    std::cout << std::endl;
    while (reply.as_string() != key) {
        std::cout << "密码错误，请选择 1重新输入密码 2为忘记密码: ";
        int option;
        std::cin >> option;
        if (option == 1) {
            std::cout << "请重新输入密码: ";
            key = cinkey();
            std::cout << std::endl;
        } else {
            forgetkey(server);
            std::cout << "请重新输入密码: ";
            key = cinkey();
            std::cout << std::endl;
        }
    }
    redis_.del({account, account + "1"});
    redis_.sync_commit();
    std::cout << "账号注销成功" << std::endl;
}
void verifycode::sendcom(const std::string& serveraccount,
                         const std::string clientaccount,
                         const std::string& subject,
                         const std::string code) {
    CURL* curl = curl_easy_init();
    std::string from = "<" + serveraccount + ">";
    std::string to = "<" + clientaccount + ">";
    std::string mail = "To: " + clientaccount +
                       "\r\n"
                       "From: " +
                       serveraccount +
                       "\r\n"
                       "Subject: " +
                       subject + "\r\n\r\n" + code;
    struct curl_slist* recipients = NULL;
    recipients = curl_slist_append(recipients, to.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.qq.com:465");
    curl_easy_setopt(curl, CURLOPT_USERNAME, serveraccount.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, "miojajsaujebdbch");
    // curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=PLAIN");
    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, mail_payload);
    curl_easy_setopt(curl, CURLOPT_READDATA, &mail);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        std::cout << "发送成功！" << std::endl;
    } else {
        std::cout << "发送失败: " << curl_easy_strerror(res) << std::endl;
    }
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
}
// int main() {
//     curl_global_init(CURL_GLOBAL_ALL);
//     verifycode c;
//     std::string server = "3541053286@qq.com";
//     int clientfd;
//     int choice;
//     while (1) {
//         std::cout << "账号管理";
//         std::cout << "1. 注册\n";
//         std::cout << "2. 密码登录\n";
//         std::cout << "3. 验证码登录\n";
//         std::cout << "4. 忘记密码（发送原密码到邮箱）\n";
//         std::cout << "5. 注销账号\n";
//         std::cout << "0. 退出\n";
//         std::cout << "请选择: ";
//         std::cin >> choice;

//         switch (choice) {
//             case 1:{
//                 c.signup();
//                 break;
//             }
               
//             case 2:{
//                 std::string account;
//                 std::cout << "请输入qq邮箱账号:";
//                 std::cin >> account;
//                 c.loginwithkey(server, clientfd, account);
//                 break;
//             }
            
//             case 3:{
//                 std::string account;
//                 std::cout << "请输入qq邮箱账号:";
//                 std::cin >> account;
//                 c.loginwithcode(server, clientfd, account);
//                 break;
//             }
                
//             case 4:{
//                 c.forgetkey(server);
//                 break;
//             }
               
//             case 5:{
//                 c.destroy(server);
//                 break;
//             }
               
//             case 0:{
//                 std::cout << "再见" << std::endl;
//                 curl_global_cleanup();
//                 return 0;
//             }
                
//             default:
//                 std::cout << "选择错误" << std::endl;
//         }
//     }
// }