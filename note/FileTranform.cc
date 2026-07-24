#pragma once
#include "choicework.h"
#include <fcntl.h>
Choicework c;
FileTransform::FileTransform(TcpClient* client) : client_(client) {}
void FileTransform::sendfile(std::string ID,std::string filepath,std::string filename,std::string filesize){
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
    id = c.gen_req_id();
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
FileContext &FileTransform::getfc(){
    return fc;
}
void FileTransform::loadfile(std::string filename,std::string filesize,std::string ID,std::string filepath){
    fc.filename = filename;
    fc.filesize = std::stoi(filesize);
    filepath = filepath + "/" + filename;
    fc.ID = ID;
    fc.recvsize = 0;
    fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if(fc.fd==-1){
        std::cout << "文件创建失败" << std::endl;
        return;
    }
    json j;
    j["cmd"] = "RETR";
    j["ID"] = ID;
    j["filename"] = filename;
    j["filesize"] = filesize;
    id = c.gen_req_id();
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