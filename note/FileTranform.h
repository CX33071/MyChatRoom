#pragma once
#include <termios.h>
#include <wait.h>
#include <condition_variable>
#include <future>
#include <queue>
#include <unordered_map>
#include "/home/cx33071/muduo-/net/TcpClient.h"
#include "json.hpp"
using json = nlohmann::json;
enum class FileState { PRASEJSON, RECV_FILE, SEND_FILE };
struct FileContext {
    FileState state = FileState::PRASEJSON;
    int fd = -1;
    std::string filename;
    uint64_t filesize = 0;
    uint64_t recvsize = 0;
    std::string ID;
};
class FileTransform{
    public:
     FileTransform(TcpClient* client);
     void sendfile(std::string ID,std::string filepath,std::string filename,std::string filesize);
     void loadfile(std::string filename,std::string filesize,std::string ID,std::string filepath);
     FileContext &getfc();
     private:
      TcpClient* client_;
      FileContext fc;
};