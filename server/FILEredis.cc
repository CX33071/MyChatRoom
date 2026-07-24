#include "FILEredis.h"
FILEredis::FILEredis(){
    redis_.connect("127.0.0.1", 6379);
    redis_.sync_commit();
    std::string sql =
        "create table if not exists fileid_info (id int auto_increment "
        "primary key,sender varchar(30),reciver varchar(30),filename "
        "varchar(30),filesize varchar(30),fileid varchar(30),status varchar(30));";
    mysql_.createinfo(sql.c_str());
}
void FILEredis::begin(std::string from,
           std::string to,
           std::string filename,std::string filesize,
           std::string ID){
    redis_.hmset("file:"+ID, {{"from", from},
                      {"to", to},
                      {"filename", filename},
                      {"filesize", filesize},{"status","uploading"}});
    redis_.sync_commit();
    std::string sql =
        "insert into fileid_info "
        "(sender,reciver,filename,filesize,fileid,status)values('" +
        from + "','" + to + "','" + filename + "','" + filesize + "','" + ID +
        "','uploading')";
    mysql_.addmsg(sql.c_str());
}
void FILEredis::setloadfinish(std::string ID){
    redis_.hset("file:" + ID, "status", "loadfinished");
    redis_.sync_commit();
    std::string sql =
        "update fileid_info set status = 'loadfinished' where fileid = '" + ID +
        "'";
    mysql_.changemsg(sql.c_str());
}
std::string FILEredis::finish(
            std::string ID){
    redis_.hset("file:"+ID, "status", "finished");
    redis_.sync_commit();
    std::string sql = "update fileid_info set status = 'finished' where fileid = '" + ID + "'";
    mysql_.changemsg(sql.c_str());
    auto s = redis_.exists({"file:" + ID});
    redis_.sync_commit();
    if (!s.get().as_integer()) {
        std::string sql1 = "select reciver from fileid_info where ID ='" + ID + "'";
        std::string to = mysql_.selectstring(sql1.c_str());
        return to;
    }
    auto f = redis_.hget("file:" + ID, "to");
    redis_.sync_commit();
    std::string to = f.get().as_string();
    return  to;
}
std::string FILEredis::getfrom(std::string ID){
    auto s = redis_.exists({"file:" + ID});
    redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql = "select sender from fileid_info where fileid ='" + ID + "'";
        std::string from=mysql_.selectstring(sql.c_str());
        return from;
    }
    auto f = redis_.hget("file:" + ID, "from");
    redis_.sync_commit();
    std::string from = f.get().as_string();
    return from;
}
std::string FILEredis::getfilesize(std::string ID){
    auto s = redis_.exists({"file:" + ID});
    redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql =
            "select filesize from fileid_info where fileid ='" + ID + "'";
        std::string filesize = mysql_.selectstring(sql.c_str());
        return filesize;
    }
    auto f = redis_.hget("file:" + ID, "filesize");
    redis_.sync_commit();
    std::string filesize = f.get().as_string();
    return filesize;
}
