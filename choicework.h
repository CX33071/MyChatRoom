#include <termios.h>
#include <wait.h>
#include <condition_variable>
#include <future>
#include <queue>
#include "/home/cx33071/muduo-/net/TcpClient.h"
#include "json.hpp"
#include <unordered_map>
#include <future>
#define RESET "\033[0m"
#define GREEN "\033[1;32m"
#define BLUEC "\033[1;34m"
#define BLUE "\033[34m"
#define PURPLE "\033[1;35m"
using json = nlohmann::json;
extern std::string account;
extern std::string password;
extern std::string verifycode;
extern std::string frienduser;
extern std::string groupname;
extern json j;
extern TcpClient::TcpConnectionPtr c_conn;
extern std::condition_variable c_cv;
extern std::mutex c_mutex;
extern std::mutex msg_mutex;
extern std::condition_variable msg_cv;
extern std::mutex active_mutex;
extern std::unordered_map<std::string, std::promise<json>> active_requests;
extern std::atomic<bool> is_login;
extern std::atomic<bool> recive;
extern std::atomic<bool> connok;
extern std::atomic<bool> running;
extern std::atomic<int> req_id;
extern std::string id;
struct Event {
    enum Type { Friendadd, GroupInvite } type;
    json data;
};
  extern std::queue<Event> event_queue;
  extern std::mutex event_mutex;
  extern std::condition_variable event_cv;
  extern std::vector<std::string> addlist;
  extern std::vector<json> invitelist;
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
      void handle_friendchat();
      void handle_friendlist();
      void handle_block();
      void handle_disblock();
      void handle_delfriend();
      void handle_creategroup();
      void handle_invite();
      void handle_exitgroup();
      void handle_addfriendmsg();
      void handle_invitemsg();
      void handle_groupchat();
      std::string gen_req_id();
     private:
      std::string cinkey();
  };