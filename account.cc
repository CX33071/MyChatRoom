#include "account.h"
std::string server = "3541053286@qq.com";
size_t mail_payload(void* ptr, size_t size, size_t nmemb, void* userp) {
    std::string* data = (std::string*)userp;
    size_t len = data->size();
    memcpy(ptr, data->c_str(), len);
    data->clear();
    return len;
}
Verifycode::Verifycode() {
    srand(time(NULL));
    redis_.connect("127.0.0.1", 6379);
}
std::string Verifycode::code() {
    std::string code;
    for (int i = 0; i < 4; i++) {
        code += ('0' + rand() % 10);
    }
    return code;
}
void Verifycode::addredis(
                          const std::string& account) {
    std::string s = code();
    redis_.setex(account + "code", 300, s);
    redis_.sync_commit();
    sendcom( account, "验证码", "您的验证码是: " + s + " 5分钟内有效");
}
bool Verifycode::signup(std::string account,std::string password) {
    auto fut = redis_.exists({account+"key"});
    redis_.sync_commit();
    int exists = fut.get().as_integer();
    if(exists){
        return false;
    }
    redis_.set(account+"key", password);
    redis_.sync_commit();
    return true;
}
bool Verifycode::verify(std::string account,std::string code) {
    auto fut = redis_.get(account + "code");
    redis_.sync_commit();
    if (fut.get().as_string() == code) {
        redis_.del({account + "code"});
        redis_.sync_commit();
        redis_.set("online" + account, "1");
        return true;
    }
    return false;
}
int Verifycode::loginwithkey(std::string account,std::string password) {
    auto fut = redis_.get(account+"key");
    redis_.sync_commit();
    auto reply = fut.get();
    if (!reply.is_string()) {
        return 1;
    }
    std::string hashkey = reply.as_string();
    if(password!=hashkey) {
        return 2;
    }
    redis_.set("online" + account, "1");
    return 0;
}
bool Verifycode::forgetkey(std::string account) {
    auto fut = redis_.exists({account+"key"});
    redis_.sync_commit();
    if (fut.get().as_integer() == 0) {
        // std::cout << "该账号并未注册" << std::endl;
        return false;
    }
    addredis(account);
    auto fut1 = redis_.get(account+"key");
    redis_.sync_commit();
    std::string hashcode = fut1.get().as_string();
    sendcom( account, "密码", "您的密码是: " + hashcode);
    return true;
}
int Verifycode::destroy(std::string account,std::string password) {
    auto fut = redis_.exists({account + "key"});
    redis_.sync_commit();
    if (fut.get().as_integer() == 0) {
        return 1;
    }
    auto fut1 = redis_.get(account+"key");
    redis_.sync_commit();
    auto reply = fut1.get();
    if (reply.as_string() !=password){
        return 2;
    }
    redis_.del({account+"key", account + "code"});
    redis_.sync_commit();
    return 0;
}
void Verifycode::sendcom(
                         const std::string clientaccount,
                         const std::string& subject,
                         const std::string code) {
    CURL* curl = curl_easy_init();
    
    std::string from = "<" + server + ">";
    std::string to = "<" + clientaccount + ">";
    std::string mail = "To: " + clientaccount +
                       "\r\n"
                       "From: " +
                       server+
                       "\r\n"
                       "Subject: " +
                       subject + "\r\n\r\n" + code;
    struct curl_slist* recipients = NULL;
    recipients = curl_slist_append(recipients, to.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.qq.com:465");
    curl_easy_setopt(curl, CURLOPT_USERNAME, server.c_str());
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
