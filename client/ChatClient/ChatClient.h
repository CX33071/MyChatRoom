#pragma once
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <wait.h>
#include <condition_variable>
#include <future>
#include <queue>
#include "database/SQlite.h"
#include <regex>
#include <readline/readline.h>
#include <dirent.h>
#include <unordered_map>
#include "muduo/net/TcpClient.h"
#include <ncurses.h>
#include <nlohmann/json.hpp>
#define RESET "\033[0m"
#define GREEN "\033[1;32m"
#define BLUEC "\033[1;34m"
#define BLUE "\033[34m"
#define PURPLE "\033[1;35m"
using json = nlohmann::json;
struct chatrecord{
    std::string sender;
    std::string reciver;
    std::string content;
};
struct filestatus{
    std::string sended;
    std::string total;
    std::string filename;
    std::string recived;
    std::string from;
    std::string id;
    std::string to;
};
struct loadfile{
    std::string from;
    std::string filename;
    std::string ID;
    bool has=false;
};
template <typename T>
bool wait_future(std::future<T>& f, T& result) {
    if (f.wait_for(std::chrono::seconds(10)) == std::future_status::timeout) {
        return false;
    }

    result = f.get();
    return true;
}
class FileClient;
class chatclient {
   private:
    TcpClient client_;
    std::vector<chatrecord> friendhistory;
    std::vector<std::string> groupnamehistory;
    std::vector<std::string> grouphistorysender;
    std::string filename;
    std::string filepath;
    std::string password;
    std::string verifycode;
    std::string frienduser;
    std::string groupname;
    json j;
    std::condition_variable chat_cv;
    std::mutex chat_mutex;

    std::atomic<bool> chatconnok=false;
    std::atomic<int> req_id=0;
    EventLoop *loop_;
    chatrecord cr;

   public:
    std::vector<std::string> grouplist;
    std::unordered_map<std::string, bool> isblocklist;
    int chat_menu_index = -1;
    std::unordered_map<std::string, int> chatmessage;
    std::unordered_map<std::string, int> groupmessage;
    static chatclient* chatptr;
    bool shortchat = true;
    std::string chatfriendname;
    std::map<std::string, std::vector<json>> chatfile;
    std::mutex active_mutex;
    std::unordered_map<std::string, std::promise<json>> active_requests;
    std::string id;
    loadfile lf;
    termios oldt;
    termios nt;
    std::vector<filestatus> uploads;
    std::vector<filestatus> downloads;
    std::map<std::string, std::vector<json>> chatuploads;
    std::map<std::string, std::vector<json>> chatdownloads;
    std::atomic<bool> stop1 = false;
    SQlite SQ;
    std::atomic<bool> running = false;
    std::string name;
    FileClient* fileclient_;
    std::unordered_map<std::string, std::queue<json>> msg_map;
    TcpClient::TcpConnectionPtr chat_conn;
    std::vector<json> addlist;
    std::vector<json> applyjoinlist;
    std::vector<json> sendfilelist;
    std::string account;
    std::atomic<bool> inchat = false;
    std::atomic<bool> groupchat = false;
    std::string current_chat;
    struct Event {
        enum Type {
            Friendadd,
            GroupInvite,
            FRIENDCHAT,
            GROUPCHAT,
            APPLYJOINGROUP,
            SENDFILE
        } type;
        json data;
    };
    std::queue<Event> event_queue;
    std::mutex event_mutex;
    std::condition_variable event_cv;
    std::atomic<bool> chatis_login=false;
    std::mutex msg_mutex;
    std::condition_variable msg_cv;
    chatclient(EventLoop* loop, const InetAddress& addr);
    void sendheart();
    void cancel_requests();
    bool is_groupmember2(std::string groupname);
    bool is_groupmember(std::string groupname,std::string account);
    void showchatmenu(int index);
    void erase_utf8(std::string& msg);
    void setfileclient(FileClient* client);
    void connectioncallback(const TcpClient::TcpConnectionPtr& conn);
    bool is_online(std::string account);
    void messagecallback(const TcpClient::TcpConnectionPtr& conn,
                         Buffer* buf,
                         Timestamp);
    void main_menu();
    void friend_menu();
    void clearchatmenu();
    void group_menu();
    void file_menu();
   static int emptyfunction(int n1, int n2);
    void select_menu();
    void msg_menu();
    int changenum(std::string s);
    std::string getaccount(std::string name);
    std::string getname(std::string account);
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
    void handle_uploadingfile();
    void handle_downingfile();
    void handle_downloadcheck();
    void handle_modifyname();
    void handle_changeowner();
    void getgrouphistory(std::string groupname);
    void handle_getfriendchat(std::string friendaccount);
    void handle_friendlist();
    void handle_block();
    std::vector<std::string> getlocalfile(std::string path);
    void handle_disblock();
    void handle_delmember();
    bool is_exists(std::string account);
    bool is_existsname(std::string name);
    bool is_blocked(std::string frienduser);
    void handle_delfriend();
    void handle_creategroup();
    void handle_exitgroup();
    void handle_groupsendfile();
    void handle_chatgroupsendfile(std::string groupname);
    void handle_chatgrouploadfile(std::string friendaccount);
    void handle_chatuploadcheck();
    void handle_chatdownloadcheck();
    void handle_downfilelist();
    void handle_chatdownfilelist();
    void handle_rechatsendfile(std::string toaccount);
    void handle_rechatloadfile(std::string fromaccount);
    void handle_regroupchatsendfile(std::string groupname);
    void handle_regroupchatloadfile(std::string groupname);
    void handle_exitlogin();
    void handle_addfriendmsg();
    void handle_applyjoinmsg();
    void handle_groupchat();
    void handle_onlinelist();
    bool is_owner(std::string account, std::string groupname);
    void handle_chatsendfile(std::string frienduser);
    void handle_blocklist();
    void handle_grouplist();
    void handle_uploadcheck();
    void handle_groupmember();
    void handle_setgroupmanager();
    void handle_sendedfile();
    void handle_sendfile();
    void handle_loadfile();
    bool is_existsgroup(std::string groupname);
    bool is_manager(std::string groupname, std::string count);
    void handle_chatloadfile(std::string from);
    int is_friend(std::string account);
    void printfmembers();
    void checkreadlineinput(char *line);
    void handle_ownergrouplist();
    int is_blockfriend(std::string account);
    std::string gen_req_id();
    std::string cinkey();
    bool isQQemail(std::string email);
    static int exechatmenu(int n1, int n2);
};
