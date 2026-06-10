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
std::string Verifycode::generatesalt() {
    std::string salt;
    for (int i = 0; i < 10; i++) {
        salt += ('0' + rand() % 10);
    }
    return salt;
}
std::string Verifycode::sha(std::string input) {
    std::reverse(input.begin(), input.end());
    return input;
}
std::string Verifycode::screctkey(std::string key) {
    std::string salt = generatesalt();
    std::string hash = sha(salt + key);
    return salt + ":" + hash;
}
std::string Verifycode::getstartkey(std::string hashkey) {
    size_t pos = hashkey.find(':');
    std::string salt = hashkey.substr(0, pos);
    std::string hash = hashkey.substr(pos + 1);
    std::reverse(hash.begin(), hash.end());
    std::string startkey = hash.substr(10);
    return startkey;
}
bool Verifycode::checkkey(const std::string& inputkey,
                          const std::string& getkeyvalue) {
    size_t pos = getkeyvalue.find(':');
    std::string salt = getkeyvalue.substr(0, pos);
    std::string hash = getkeyvalue.substr(pos + 1);
    std::string inputhash = sha(salt + inputkey);
    return inputhash == hash;
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
    redis_.setex(account + "1", 300, s);
    redis_.sync_commit();
    sendcom( account, "验证码", "您的验证码是: " + s + " 5分钟内有效");
}
bool Verifycode::signup(std::string account,std::string password) {
    auto fut = redis_.exists({account});
    redis_.sync_commit();
    int exists = fut.get().as_integer();
    if(exists){
        return false;
    }
    std::string finalkey = screctkey(password);
    redis_.set(account, finalkey);
    redis_.sync_commit();
    return true;
}
bool Verifycode::verify(std::string account,std::string code) {
    auto fut = redis_.get(account + "1");
    redis_.sync_commit();
    if (fut.get().as_string() == code) {
        redis_.del({account + "1"});
        redis_.sync_commit();
        return true;
    }
    return false;
}
bool Verifycode::loginwithkey(std::string account,std::string password) {
    auto fut = redis_.get(account);
    redis_.sync_commit();
    auto reply = fut.get();
    if (!reply.is_string()) {
        // std::cout << "该账号不存在，请先注册" << std::endl;
        exit(1);
    }
    std::string hashkey = reply.as_string();
    if(!checkkey(password, hashkey)) {
        return false;
    }
    redis_.set("online" + account, "1");
    return true;
}
bool Verifycode::forgetkey(std::string account) {
    auto fut = redis_.exists({account});
    redis_.sync_commit();
    if (fut.get().as_integer() == 0) {
        // std::cout << "该账号并未注册" << std::endl;
        return false;
    }
    addredis(account);
    auto fut1 = redis_.get(account);
    redis_.sync_commit();
    std::string hashcode = fut1.get().as_string();
    std::string truecode = getstartkey(hashcode);
    sendcom( account, "密码", "您的密码是: " + truecode);
    return true;
}
bool Verifycode::destroy(std::string account,std::string password) {
    auto fut = redis_.get(account);
    redis_.sync_commit();
    auto reply = fut.get();
    if (reply.as_string() !=password){
        return false;
    }
    redis_.del({account, account + "1"});
    redis_.sync_commit();
    return true;
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
