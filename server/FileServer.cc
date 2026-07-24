
#pragma once
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
#include "FILEredis.h"
#include "friend.h"
#include "group.h"
#include "json.hpp"
#include <fcntl.h>
#include "FileServer.h"
using json = nlohmann::json;
FileServer::FileServer(EventLoop* loop,
                       std::string name,
                       const InetAddress& addr)
    : server_(loop, name, addr) {
    server_.setThreadNum(4);
    server_.setConnectionCallback(
        [this](const TcpConnectionPtr& conn) { connectioncallback(conn); });

    server_.setMessageCallback(
        [this](const TcpConnectionPtr& conn, Buffer* buf, Timestamp t) {
            messagecallback(conn, buf, t);
        });
    LOG_INFO << "服务器启动";
    server_.start();
}
void FileServer::connectioncallback(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        LOG_INFO << "有一个fileclient文件客户端连接成功";
        FileContext fc;
        uploadmap_.emplace(conn, fc);
    } else {
        LOG_INFO << "有一个fileclient文件客户端下线了";
        uploadmap_.erase(conn);
    }
}
void FileServer::messagecallback(const TcpConnectionPtr& conn,
                                 Buffer* buf,
                                 Timestamp) {
     FileContext& fc = uploadmap_[conn];
     if (fc.state == FileState::PRASEJSON) {
         const char* pos = buf->findn();
         if (pos) {
             std::string msg(buf->peek(), pos - buf->peek());
             buf->retrieveUntil(pos + 1);

             json j;
             j = json::parse(msg);
             std::string cmd = j["cmd"];
             if(cmd=="STOR"){
                 std::string filename = j["filename"];
                 std::string filepath = "./file/" + filename;
                 fc.ID = j["ID"];
                 fc.filename = j["filename"];
                 std::cout <<"收到"<< j.dump(4) << std::endl;
                 std::string ss = j["filesize"];
                 uint64_t filesize = std::stoi(ss);
                 fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY|O_TRUNC,0644);
                 if(fc.fd==-1){
                     std::cout << "文件创建失败" << std::endl;
                 }
                 std::cout << "已创建文件在" << filepath << std::endl;
                 fc.filesize = filesize;
                 fc.recvsize = 0;
                 fc.state = FileState::RECV_FILE;
                 json res;
                 res["cmd"] = "STORres";
                 res["ok"] = true;
                 std::string request_id = j.value("request_id", "");
                 if (!request_id.empty()) {
                     res["request_id"] = request_id;
                 }
                 conn->send(res.dump() + "\n");
                 groupsend = false;
             }
             if(cmd=="groupSTOR"){
                 std::string filename = j["filename"];
                 std::string filepath = "./file/" + filename;
                 fc.ID = j["ID"];
                 fc.filename = j["filename"];
                 std::cout << "收到" << j.dump(4) << std::endl;
                 std::string ss = j["filesize"];
                 uint64_t filesize = std::stoi(ss);
                 fc.fd =
                     open(filepath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                 if (fc.fd == -1) {
                     std::cout << "文件创建失败" << std::endl;
                 }
                 std::cout << "已创建文件在" << filepath << std::endl;
                 fc.filesize = filesize;
                 fc.recvsize = 0;
                 fc.state = FileState::RECV_FILE;
                 json res;
                 res["cmd"] = "groupSTORres";
                 res["ok"] = true;
                 std::string request_id = j.value("request_id", "");
                 if (!request_id.empty()) {
                     res["request_id"] = request_id;
                 }
                 conn->send(res.dump() + "\n");
                 groupsend = true;
             }
             if(cmd=="RETR"){
                 std::string ID = j["ID"];
                 std::string filesize = j["filesize"];
                 std::string filename = j["filename"];
                 json res;
                 res["cmd"] = "RETRres";
                 std::string request_id = j.value("request_id", "");
                 if (!request_id.empty()) {
                     res["request_id"] = request_id;
                 }
                 std::string filepath = "./file/" + filename;
                 std::cout << "filepath: " << filepath << std::endl;
                 int fd = open(filepath.c_str(), O_RDONLY);
                 if(fd==-1){
                     std::cout << "./file本地找不到文件" << std::endl;
                 }
                 conn->send(res.dump() + "\n");
                 ssize_t n;
                 char buf[4096];
                 while ((n = read(fd, buf, sizeof(buf))) > 0) {
                     conn->send(std::string(buf, n));
                 }
                 close(fd);
                 std::cout << "文件上传给客户端完成" << std::endl;
             }
             if(cmd=="to_clientfinish"){
                 json res;
                 std::string request_id = j.value("request_id", "");
                 if (!request_id.empty()) {
                     res["request_id"] = request_id;
                 }
                 conn->send(res.dump() + '\n');
             }
         }
     } else if (fc.state == FileState::RECV_FILE) {
         size_t len = buf->readableBytes();
         if(len==0){
             return;
         }
         size_t remain = fc.filesize - fc.recvsize;
         if (len > remain) {
             len = remain;
         }
             write(fc.fd, buf->peek(), len);
             buf->retrieve(len);
             fc.recvsize += len;
             if (fc.recvsize== fc.filesize) {
                 close(fc.fd);
                 fc.fd = -1;
                 fc.state = FileState::PRASEJSON;
                 fc.recvsize = 0;
                 fc.filesize = 0;
                 std::cout << "文件接收完成" << std::endl;
                 json res;
                 if(groupsend){
                     res["cmd"] = "groupSTOR_ok";
                 }else{
                     res["cmd"] = "STOR_ok";
                 }
                 res["ID"] = fc.ID;
                 res["filename"] = fc.filename;
                 conn->send(res.dump() + "\n");
                 fc.recvsize = 0;
             }
     } else {
     }
 }