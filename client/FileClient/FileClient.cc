#include "FileClient.h"
#include "ChatClient/ChatClient.h"
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
void FileClient::fileprogress(long long cur,long long filesize,Timestamp begin){
    int total = 40;
    double curpercent = (double)cur / filesize;
    int pos = curpercent * total;
    std::cout << GREEN << "\r[";
    for (int i = 0; i < total;i++){
        if(i<pos){
            std::cout << "=";
        }else{
            std::cout << " ";
        }
    }
    std::cout << "]";
    std::cout<<(int)(curpercent*100)<<"%";
    Timestamp now = Timestamp::now();
    Timestamp t;
    double speed = cur / 1000000.0 / t.timeDifference(now, begin);
    double uploaded = (filesize - cur) / 1000000.0;
    double res = uploaded / speed;
    std::cout << std::fixed << std::setprecision(1) << speed << " MB/s ";
    std::cout << "剩余" << std::setprecision(2) << res << " 秒";
    std::cout << RESET << std::flush;
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
            if (ischatsend) {
                j1["chatsend"] = "1";
            }else{
                j1["chatsend"] = "2";
            }
            id = gen_req_id();
            j1["request_id"] = id;
            std::promise<json> p;
            std::future<json> f = p.get_future();
            {
                std::lock_guard<std::mutex> lock(active_mutex);
                active_requests[id] = std::move(p);
            }
            chatclient_->chat_conn->send(j1.dump() + '\n');
        } else if (cmd == "loadfileres") {
        } else if (cmd == "groupSTOR_ok") {
            std::string ID = j["ID"];
            std::string filename = j["filename"];
            json j1;
            j1["cmd"] = "groupsendfile_finish";
            if(groupchatsend){
                j1["groupchat"] = "1";
            }else{
                j1["groupchat"] = "2";
            }
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
            size_t downloaded = fc.filesize - fc.downsize;
            if (len > downloaded) {
                len = downloaded;
            }
            write(fc.fd, buf->peek(), len);
            buf->retrieve(len);
            fc.downsize += len;
            fc.recvsize += len;
            json j1;
            j1["cmd"] = "update_downsize";
            j1["fileid"] = fc.ID;
            j1["downsize"] = std::to_string(fc.downsize);
            conn->send(j1.dump() + '\n');
            fileprogress(fc.recvsize, fc.filesize, begin1);
            if (fc.recvsize == fc.filesize) {
                close(fc.fd);
                fc.recvsize = 0;
                fc.filesize = 0;
                fc.downsize = 0;
                json res;
                res["cmd"] = "RETR_ok";
                res["ID"] = fc.ID;
                if(ischatload){
                    res["chatload"] = "1";
                }else{
                    res["chatload"] = "2";
                }
                res["filename"] = fc.filename;
                res["account"] = chatclient_->account;
                chatclient_->chat_conn->send(res.dump() + "\n");
                fc.state = FileState::PRASEJSON;
                json j3;
                j3["cmd"]="Load_finish";
                j3["request_id"] = fileloadID;
                file_conn->send(j3.dump() + '\n');
            }
        }
}
void FileClient ::sendfile(std::string ID,
                           std::string filepath,
                           std::string filename,
                           std::string filesize,bool ischatsend1){
    if(ischatsend1){
        ischatsend = true;
    }
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
    char* end;
    long long num = strtoll(filesize.c_str(), &end, 10);
    Timestamp begin = Timestamp::now();
    bool cancel = false;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if(chatclient_->stop1){
            cancel = true;
            break;
        }
        if(file_conn){
            file_conn->send(std::string(buf, n));
        }
        total+=n;
        fileprogress(total, num,begin);
    }
    close(fd);
    std::cout << std::endl;
    if(cancel){
        std::cout << PURPLE << "上传取消" << RESET << std::endl;
    }else{
        std::cout << PURPLE << "\n文件上传完成" << RESET << std::endl;
    }
                           }
void FileClient::groupsendfile(std::string ID,
                  std::string filepath,
                  std::string filename,
                  std::string filesize,bool groupchat){
    groupchatsend = groupchat;
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
    char* end;
    long long num = strtoll(filesize.c_str(), &end, 10);
    bool cancel = false;
    Timestamp begin = Timestamp::now();
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if(chatclient_->stop1){
            cancel = true;
            break;
        }
        if(file_conn){
            file_conn->send(std::string(buf, n));
        }
        total += n;
        fileprogress(total, num, begin);
    }
    close(fd);
    std::cout << std::endl;
    if (cancel) {
        std::cout << PURPLE << "上传取消" << RESET << std::endl;
    } else {
        std::cout << PURPLE << "\n文件上传完成" << RESET << std::endl;
    }
                  }
int FileClient ::loadfile(std::string filename,
                           std::string filesize,
                           std::string ID,
                           std::string filepath,bool ischatload1){
    if (ischatload1) {
        ischatload = true;
    }
    fc.filename = filename;
    fc.filesize = std::stoi(filesize);
    filepath = filepath + "/" + filename;
    fc.ID = ID;
    fc.recvsize = 0;
    fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fc.fd == -1) {
        std::cout << "文件创建失败" << std::endl;
        std::cerr << "文件创建失败: " << std::strerror(errno)
                  << " (errno=" << errno << ")" << std::endl;
        return -1;
    }
    json j;
    j["cmd"] = "RETR";
    j["ID"] = ID;
    j["filename"] = filename;
    j["filesize"] = filesize;
    id = gen_req_id();
    j["request_id"] = id;
    fileloadID = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    file_conn->send(j.dump() + '\n');
    begin1 = Timestamp::now();
    fc.state = FileState::RECV_FILE;
    json res = f.get();
    std::cout << std::endl;
    std::cout << PURPLE << "文件下载成功!" << RESET << std::endl;
    return 0;
}
int  FileClient::uploadedsendfile(std::string fileid, std::string uploaded,std::string filepath,std::string filename,std::string filesize,bool b,bool ischatsend1,bool groupchat){
    ischatsend = ischatsend1;
    groupchatsend = groupchat;
    int fd = open(filepath.c_str(), O_RDONLY);
    char* end = 0;
    long uploadedsize = std::strtol(uploaded.c_str(), &end, 10);
    lseek(fd, uploadedsize, SEEK_SET);
    json j;
    if(b){
        j["ok"] = "group";
    }else{
        j["ok"] = "friend";
    }
    j["cmd"]="uploadedSTOR";
    j["filename"] = filename;
    j["recivesize"] = uploaded;
    j["filesize"] = filesize;
    j["ID"]=fileid;
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
    end = 0;
    Timestamp begin = Timestamp::now();
    bool cancel = false;
    long num = strtol(filesize.c_str(), &end, 10);
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (chatclient_->stop1) {
            cancel = true;
            break;
        }
        if (file_conn) {
            file_conn->send(std::string(buf, n));
        }
        total += n;
        fileprogress(total, num, begin);
    }
    close(fd);
    std::cout << std::endl;
    if (cancel) {
        std::cout << PURPLE << "上传取消" << RESET << std::endl;
        return -1;
    } else {
        std::cout << PURPLE << "\n文件上传完成" << RESET << std::endl;
        return 0;
    }
}
int FileClient::reloadfile(std::string filename,
               std::string filesize,
               std::string ID,
               std::string filepath,
               std::string downsize,bool ischatload1){
    ischatload = ischatload1;
    fc.filename = filename;
    fc.filesize = std::stoi(filesize);
    filepath = filepath + "/" + filename;
    fc.ID = ID;
    fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY , 0644);
    if (fc.fd == -1) {
        std::cout << "文件创建失败" << std::endl;
        std::cerr << "文件创建失败: " << std::strerror(errno)
                  << " (errno=" << errno << ")" << std::endl;
        return -1;
    }
    long long downsize1 = stoll(downsize);
    fc.recvsize = downsize1;
    fc.downsize = downsize1;
    lseek(fc.fd, downsize1, SEEK_SET);
    json j;
    j["cmd"] = "reRETR";
    j["ID"] = ID;
    j["filename"] = filename;
    j["filesize"] = filesize;
    j["downsize"] = downsize;
    id = gen_req_id();
    j["request_id"] = id;
    fileloadID = id;
    std::promise<json> p;
    std::future<json> f = p.get_future();
    {
        std::lock_guard<std::mutex> lock(active_mutex);
        active_requests[id] = std::move(p);
    }
    file_conn->send(j.dump() + '\n');
    begin1 = Timestamp::now();
    fc.state = FileState::RECV_FILE;
    json res = f.get();
    std::cout << std::endl;
    std::cout << PURPLE << "文件下载成功!" << RESET << std::endl;
    return 0;
               }
               