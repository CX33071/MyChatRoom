
#pragma once
#include <assert.h>
#include <unordered_map>
#include <unordered_set>
#include "/home/cx33071/muduo-/base/logger.h"
#include "/home/cx33071/muduo-/net/TcpServer.h"
#include "FILEredis.h"
#include "friend.h"
#include "group.h"
#include <nlohmann/json.hpp>
#include <fcntl.h>
#include "FileServer.h"
using json = nlohmann::json;
FileServer::FileServer(EventLoop* loop,
                       std::string name,
                       const InetAddress& addr)
    : server_(loop, name, addr) {
    redis_.connect("127.0.0.1", 6379);
    redis_.sync_commit();
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
       while(true){
           const char* pos = buf->findn();
           if(!pos){
               break;
           }
               std::string msg(buf->peek(), pos - buf->peek());
               buf->retrieveUntil(pos + 1);
               json j;
               j = json::parse(msg);
               std::string cmd = j["cmd"];
               if (cmd == "STOR") {
                   std::string filename = j["filename"];
                   std::string filepath = "./file/" + filename;
                   fc.ID = j["ID"];
                   fc.filename = j["filename"];
                   std::cout << "收到" << j.dump(4) << std::endl;
                   std::string ss = j["filesize"];
                   uint64_t filesize = std::stoi(ss);
                   fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY | O_TRUNC,
                                0644);
                   if (fc.fd == -1) {
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
               if (cmd == "uploadedSTOR") {
                   std::string filename = j["filename"];
                   std::string filesize = j["filesize"];
                   std::string recivesize = j["recivesize"];
                   std::string filepath = "./file/" + filename;
                   fc.ID = j["ID"];
                   fc.filename = filename;
                   fc.filesize = std::stoi(filesize);
                   fc.recvsize = std::stoi(recivesize);
                   fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY, 0644);
                   lseek(fc.fd, fc.recvsize, SEEK_SET);
                   fc.state = FileState::RECV_FILE;
                   json res;
                   res["cmd"] = "uploadedsendres";
                   std::string request_id = j.value("request_id", "");
                   if (!request_id.empty()) {
                       res["request_id"] = request_id;
                   }
                   conn->send(res.dump() + "\n");
                   if (j["ok"] == "group") {
                       groupsend = true;
                   } else {
                       groupsend = false;
                   }
               }
               if (cmd == "groupSTOR") {
                   std::string filename = j["filename"];
                   std::string filepath = "./file/" + filename;
                   fc.ID = j["ID"];
                   fc.filename = j["filename"];
                   std::cout << "收到" << j.dump(4) << std::endl;
                   std::string ss = j["filesize"];
                   uint64_t filesize = std::stoi(ss);
                   fc.fd = open(filepath.c_str(), O_CREAT | O_WRONLY | O_TRUNC,
                                0644);
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
               if (cmd == "RETR") {
                   std::string ID = j["ID"];
                   redis_.hset("file:" + ID, "status", "downing");
                   redis_.hset("file:" + ID, "downsize", "0");
                   redis_.sync_commit();
                   std::string sql =
                       "update fileid_info set status = "
                       "'downing',downloadedsize = '0' where id = '" +
                       ID + "'";
                   mysql_.changemsg(sql.c_str());
                   std::string filesize = j["filesize"];
                   std::string filename = j["filename"];
                   std::string filepath = "./file/" + filename;
                   int fd = open(filepath.c_str(), O_RDONLY);
                   if (fd == -1) {
                       std::cout << "./file本地找不到文件" << std::endl;
                       return;
                   }
                   ssize_t n;
                   char buf[4096];
                   while ((n = read(fd, buf, sizeof(buf))) > 0) {
                       conn->send(std::string(buf, n));
                   }
                   close(fd);
                   fc.state = FileState::PRASEJSON;
                   std::cout << "发给客户端完成，JSON模式开启" << std::endl;
               }
               if (cmd == "reRETR") {
                   std::string ID = j["ID"];
                   std::string filesize = j["filesize"];
                   std::string filename = j["filename"];
                   std::string downsize = j["downsize"];
                   std::string filepath = "./file/" + filename;
                   int fd = open(filepath.c_str(), O_RDONLY);
                   if (fd == -1) {
                       std::cout << "./file本地找不到文件" << std::endl;
                   }
                   ssize_t n;
                   char buf[4096];
                   long long downsize1 = std::stoll(downsize);
                   lseek(fd, downsize1, SEEK_SET);
                   while ((n = read(fd, buf, sizeof(buf))) > 0) {
                       conn->send(std::string(buf, n));
                   }
                   close(fd);
                   std::cout << "文件上传给客户端完成" << std::endl;
                   fc.state = FileState::PRASEJSON;
               }
               if (cmd == "update_downsize") {
                   std::string fileid = j["fileid"];
                   std::string downsize = j["downsize"];
                   redis_.hset("file:" + fileid, "downsize", downsize);
                   redis_.sync_commit();
                   std::string sql =
                       "update fileid_info set downloadedsize ='" + downsize +
                       "'where id ='" + fileid + "'";
                   mysql_.changemsg(sql.c_str());
               }
               if (cmd == "Load_finish") {
                   json res;
                   std::string request_id = j.value("request_id", "");
                   if (!request_id.empty()) {
                       res["request_id"] = request_id;
                   }
                   std::cout << "进入Load_finish" << std::endl;
                   std::cout << "收到fileloadID=" << request_id << std::endl;
                   res["cmd"] = "loadfileres";
                   conn->send(res.dump() + '\n');
               }
               if (cmd == "to_clientfinish") {
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
         size_t uploaded = fc.filesize - fc.recvsize;
         if (len > uploaded) {
             len = uploaded;
         }
             write(fc.fd, buf->peek(), len);
             buf->retrieve(len);
             fc.recvsize += len;
             redis_.hset("file:" + fc.ID, "uploadedsize",std::to_string(fc.recvsize));
             redis_.sync_commit();
             std::string sql = "update fileid_info set uploadedsize ='" +
                               std::to_string(fc.recvsize) +
                               "'where id = '" + fc.ID + "'";
             mysql_.changemsg(sql.c_str());
             if (fc.recvsize == fc.filesize) {
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
                 close(fc.fd);
                 fc.fd = -1;
             }
     } else {
     }
 }