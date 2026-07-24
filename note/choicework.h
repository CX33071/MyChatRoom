#include <termios.h>
#include <wait.h>
#include <condition_variable>
#include <future>
#include <queue>
#include "/home/cx33071/muduo-/net/TcpClient.h"
#include "json.hpp"
#include "FileTranform.h"
#include <unordered_map>
#define RESET "\033[0m"
#define GREEN "\033[1;32m"
#define BLUEC "\033[1;34m"
#define BLUE "\033[34m"
#define PURPLE "\033[1;35m"
using json = nlohmann::json;
extern std::atomic<bool> inchat;
extern std::atomic<bool> groupchat;
extern std::string current_chat;
extern std::vector<std::string> friendhistory;
extern std::vector<std::string> groupnamehistory;
extern std::string filename;
extern std::string filepath;
extern std::string account;
extern std::string password;
extern std::string verifycode;
extern std::string frienduser;
extern std::string groupname;
extern FileTransform* ftp_;
extern json j;
extern TcpClient::TcpConnectionPtr chat_conn;
extern std::condition_variable chat_cv;
extern std::mutex chat_mutex;
extern TcpClient::TcpConnectionPtr file_conn;
extern std::condition_variable file_cv;
extern std::mutex file_mutex;
extern std::atomic<bool> fileis_login;
extern std::atomic<bool> fileconnok;
extern std::mutex msg_mutex;
extern std::condition_variable msg_cv;
extern std::mutex active_mutex;
extern std::unordered_map<std::string, std::promise<json>> active_requests;
extern std::atomic<bool> chatis_login;
extern std::atomic<bool> recive;
extern std::atomic<bool> chatconnok;
extern std::atomic<bool> running;
extern std::atomic<int> req_id;
extern std::string id;

struct Event {
    enum Type { Friendadd, GroupInvite ,FRIENDCHAT,GROUPCHAT,APPLYJOINGROUP,SENDFILE} type;
    json data;
};
  extern std::queue<Event> event_queue;
  extern std::mutex event_mutex;
  extern std::condition_variable event_cv;
  extern std::vector<std::string> addlist;
  extern std::vector<json> applyjoinlist;
  extern std::vector<json> sendfilelist;
  class Choicework {
     public:
      void main_menu();
      void friend_menu();
      void handle_signup();
      void handle_login_code();
      void handle_login_key();
      void handle_forget_key();
      void handle_destory();
      void handle_exit();
      void handle_addfriend();
      void handle_delgroup();
      void handle_applyjoingroup();
      void handle_friendchat();
      void getgrouphistory(std::string groupname);
      void handle_friendlist();
      void handle_block();
      void handle_disblock();
      void handle_delmember();
      void handle_delfriend();
      void handle_creategroup();
      void handle_exitgroup();
      void handle_exitlogin();
      void handle_addfriendmsg();
      void handle_applyjoinmsg();
      void handle_groupchat();
      void handle_onlinelist();
      void handle_blocklist();
      void handle_grouplist();
      void handle_groupmember();
      void handle_setgroupmanager();
      void handle_sendedfile();
      void handle_sendfile();
      void handle_loadfile();
      bool is_existsgroup(std::string groupname);
      bool is_manager(std::string groupname, std::string count);
      bool is_groupmember(std::string groupname,std::string count);
      int is_friend(std::string account);
      void printfmembers();

      int is_blockfriend(std::string account);
      std::string gen_req_id();
      void setfiletransform(FileTransform* f);

     private:
      std::string cinkey();
  };