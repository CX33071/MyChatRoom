#include "FileClient.h"
#include "ChatClient.h"
FileClient ::FileClient(EventLoop* loop, const InetAddress& addr)
    : client_(loop, addr) {
    client_.setConnectionCallback(
        [this](const TcpClient::TcpConnectionPtr& conn) {
            connectioncallback(conn);
        });

    client_.setMessageCallback(
        [this](const TcpClient::TcpConnectionPtr& conn, Buffer* buf,
               Timestamp t) { messagecallback(conn, buf, t); });
    client_.connect();
}
void FileClient::setchatclient(chatclient*client){
    chatclient_ = client;
}
std::string FileClient::gen_req_id(){
    return std::to_string(++req_id);
}
void FileClient ::connectioncallback(const TcpClient::TcpConnectionPtr& conn) {
    if (conn->connected()) {
        file_conn = conn;
        fileconnok = true;
    } else {
        // std::cout << "fileclientclose！" << std::endl;
        file_conn.reset();
        fileconnok = false;
    }
}
void FileClient ::messagecallback(const TcpClient::TcpConnectionPtr& conn,
                                  Buffer* buf,
                                  Timestamp) {
        if (fc.state == FileState::PRASEJSON) {
            if (!buf->findn()) {
                return;
            }
            std::string msg = buf->returnstring();
            json j = json::parse(msg);
            id = j.value("request_id", "");
            if (!id.empty()) {
                std::lock_guard<std::mutex> lock(active_mutex);
                auto it = active_requests.find(id);
                if (it != active_requests.end()) {
                    it->second.set_value(j);
                    active_requests.erase(it);
                }
            }
            std::string cmd = j["cmd"];
            if (cmd == "STOR_ok") {
                std::string ID = j["ID"];
                std::string filename = j["filename"];
                json j1;
                j1["cmd"] = "sendfile_finish";
                j1["ID"] = ID;
                j1["filename"] = filename;
                id =gen_req_id();
                j1["request_id"] = id;
                std::promise<json> p;
                std::future<json> f = p.get_future();
                {
                    std::lock_guard<std::mutex> lock(active_mutex);
                    active_requests[id] = std::move(p);
                }
                chatclient_->chat_conn->send(j1.dump() + '\n');
            } else if (cmd == "groupSTOR_ok") {
                std::string ID = j["ID"];
                std::string filename = j["filename"];
                json j1;
                j1["cmd"] = "groupsendfile_finish";
                j1["ID"] = ID;
                j1["filename"] = filename;
                id = gen_req_id();
                j1["request_id"] = id;
                std::promise<json> p;
                std::future<json> f = p.get_future();
                {
                    std::lock_guard<std::mutex> lock(active_mutex);
                    active_requests[id] = std::move(p);
                }
                chatclient_->chat_conn->send(j1.dump() + '\n');
                // std::cout<<PURPLE << "文件上传成功!" <<RESET<< std::endl;
            } else if (cmd == "RETRres") {
                fc.state = FileState::RECV_FILE;
            } else if (cmd == "sendfinishtofileclient") {
                json j1;
                std::string request_id = j.value("request_id", "");
                if (!request_id.empty()) {
                    j1["request_id"] = request_id;
                }
                j1["cmd"] = "to_clientfinish";
                file_conn->send(j1.dump() + '\n');
            }
        } else if (fc.state == FileState::RECV_FILE) {
            size_t len = buf->readableBytes();
            if (len == 0) {
                return;
            }
            size_t remain = fc.filesize - fc.recvsize;
            if (len > remain) {
                len = remain;
            }
            write(fc.fd, buf->peek(), len);
            buf->retrieve(len);
            fc.recvsize += len;
            if (fc.recvsize == fc.filesize) {
                close(fc.fd);
                fc.state = FileState::PRASEJSON;
                fc.recvsize = 0;
                fc.filesize = 0;
                json res;
                res["cmd"] = "RETR_ok";
                res["ID"] = fc.ID;
                res["filename"] = fc.filename;
                res["account"] = chatclient_->account;
                // id =gen_req_id();
                // res["request_id"] = id;
                // std::promise<json> p;
                // std::future<json> f = p.get_future();
                // {
                //     std::lock_guard<std::mutex> lock(active_mutex);
                //     active_requests[id] = std::move(p);
                // }
                chatclient_->chat_conn->send(res.dump() + "\n");
                fc.state = FileState::PRASEJSON;
                // json res1 = f.get();
                // std::cout << PURPLE << "文件下载完成!" << RESET << std::endl;
            }
        }
}
void FileClient ::sendfile(std::string ID,
                           std::string filepath,
                           std::string filename,
                           std::string filesize){
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }
    json j;
    j["cmd"] = "STOR";
    j["filename"] = filename;
    j["filesize"] = filesize;
    j["ID"] = ID;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    file_conn->send(j.dump() + '\n');
    json res = f.get();
    char buf[4096];
    ssize_t n;
    size_t total = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        file_conn->send(std::string(buf, n));
    }
    close(fd);
    std::cout << PURPLE << "文件上传完成" << RESET << std::endl;
                           }
void FileClient::groupsendfile(std::string ID,
                  std::string filepath,
                  std::string filename,
                  std::string filesize){
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }
    json j;
    j["cmd"] = "groupSTOR";
    j["filename"] = filename;
    j["filesize"] = filesize;
    j["ID"] = ID;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    file_conn->send(j.dump() + '\n');
    json res = f.get();
    char buf[4096];
    ssize_t n;
    size_t total = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        file_conn->send(std::string(buf, n));
    }
    close(fd);
    std::cout << PURPLE << "文件上传完成" << RESET << std::endl;
                  }
void FileClient ::loadfile(std::string filename,
                           std::string filesize,
                           std::string ID,
                           std::string filepath){
    fc.filename = filename;
    fc.filesize = std::stoi(filesize);
    filepath = filepath + "/" + filename;
    fc.ID = ID;
    fc.recvsize = 0;
    fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fc.fd == -1) {
        std::cout << "文件创建失败" << std::endl;
        return;
    }
    json j;
    j["cmd"] = "RETR";
    j["ID"] = ID;
    j["filename"] = filename;
    j["filesize"] = filesize;
    id = gen_req_id();
    j["request_id"] = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    file_conn->send(j.dump() + '\n');
    json res = f.get();
                           }

