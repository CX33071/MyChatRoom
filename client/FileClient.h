#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <wait.h>
#include <condition_variable>
#include <future>
#include <queue>
#include <unordered_map>
#include "/home/cx33071/muduo-/net/TcpClient.h"
#include "json.hpp"
using json = nlohmann::json;
class chatclient;
class FileClient {
   private:
    std::atomic<int> req_id = 0;
    std::condition_variable file_cv;
    std::mutex file_mutex;
    std::atomic<bool> fileis_login=false;
    std::atomic<bool> fileconnok=false;
    std::string id;
    std::mutex active_mutex;
    std::unordered_map<std::string, std::promise<json>> active_requests;
    enum class FileState { PRASEJSON, RECV_FILE, SEND_FILE };
    struct FileContext {
        FileState state = FileState::PRASEJSON;
        int fd = -1;
        std::string filename;
        uint64_t filesize = 0;
        uint64_t recvsize = 0;
        std::string ID;
    };
    TcpClient client_;
    FileContext fc;
    Timestamp begin1;

   public:
   chatclient* chatclient_;
   std::string fileloadID;
   TcpClient::TcpConnectionPtr file_conn;
   FileClient(EventLoop* loop, const InetAddress& addr);
   void setchatclient(chatclient* client);
   void connectioncallback(const TcpClient::TcpConnectionPtr& conn);
   void messagecallback(const TcpClient::TcpConnectionPtr& conn,
                        Buffer* buf,
                        Timestamp);
   void fileprogress(long long cur, long long filesize,Timestamp t);
   void sendfile(std::string ID,
                 std::string filepath,
                 std::string filename,
                 std::string filesize);
   void groupsendfile(std::string ID,
                      std::string filepath,
                      std::string filename,
                      std::string filesize);
   int loadfile(std::string filename,
                 std::string filesize,
                 std::string ID,
                 std::string filepath);
   std::string gen_req_id();
   void uploadedsendfile(std::string fileid, std::string uploaded,std::string filepath,std::string filename,std::string filesize,bool b);
};