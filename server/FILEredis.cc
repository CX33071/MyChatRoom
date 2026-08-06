#include "FILEredis.h"
FILEredis::FILEredis(){
    redis_.connect("127.0.0.1", 6379);
    redis_.sync_commit();
    std::string sql =
        "create table if not exists fileid_info (id int auto_increment "
        "primary key,sender varchar(30),reciver varchar(30),filename "
        "varchar(300),filesize varchar(300),status varchar(30),uploadedsize varchar(300) default '0',filepath varchar(300));";
    mysql_.createinfo(sql.c_str());
}
std::string FILEredis::begin(std::string from,
           std::string to,
           std::string filename,std::string filesize,std::string filepath){
    std::string sql =
        "insert into fileid_info "
        "(sender,reciver,filename,filesize,status,filepath)values('" +
        from + "','" + to + "','" + filename + "','" + filesize +
        "','uploading','"+filepath+"')";
    mysql_.addmsg(sql.c_str());
    sql = "select last_insert_id()";
    std::string s=mysql_.selectstring(sql.c_str());
    redis_.hmset("file:"+s, {{"from", from},
                           {"to", to},
                           {"filename", filename},
                           {"filesize", filesize},
                           {"status", "uploading"},{"filepath",filepath}});
    redis_.sync_commit();
    return s;
}
void FILEredis::setloadfinish(std::string ID){
    redis_.hset("file:"+ID, "status", "loadfinished");
    redis_.sync_commit();
    std::string sql =
        "update fileid_info set status = 'loadfinished' where id = '" + ID +
        "'";
    mysql_.changemsg(sql.c_str());
}
std::string FILEredis::finish(
            std::string ID){
    redis_.hset("file:"+ID, "status", "finished");
    redis_.sync_commit();
    std::string sql = "update fileid_info set status = 'finished' where id = '" + ID + "'";
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
        std::string sql = "select sender from fileid_info where id ='" + ID + "'";
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
            "select filesize from fileid_info where id ='" + ID + "'";
        std::string filesize = mysql_.selectstring(sql.c_str());
        return filesize;
    }
    auto f = redis_.hget("file:" + ID, "filesize");
    redis_.sync_commit();
    std::string filesize = f.get().as_string();
    return filesize;
}
std::string FILEredis::getuploaded(std::string fileid){
    auto s = redis_.exists({"file:" + fileid});
    redis_.sync_commit();
    if(!s.get().as_integer()){
        std::string sql="select uploadedsize from fileid_info where id ='"+fileid+"'and status ='uploading'";
        std::string uploaded = mysql_.selectstring(sql.c_str());
        return uploaded;
    }
    auto f = redis_.hget("file:" + fileid, "uploadedsize");
    redis_.sync_commit();
    std::string uploaded = f.get().as_string();
    return uploaded;
}
std::string FILEredis::getto(std::string ID){
    auto s = redis_.exists({"file:" + ID});
    redis_.sync_commit();
    if (!s.get().as_integer()) {
        std::string sql =
            "select reciver from fileid_info where id ='" + ID + "'";
        std::string from = mysql_.selectstring(sql.c_str());
        return from;
    }
    auto f = redis_.hget("file:" + ID, "to");
    redis_.sync_commit();
    std::string from = f.get().as_string();
    return from;
}
std::vector<filestatusserver>FILEredis::getuploading(std::string sender){
    std::string sql =
        "select filename from fileid_info where sender ='" + sender + "'and status = 'uploading'";
    std::vector<std::string> filenameres = mysql_.selectmul(sql.c_str());
    std::string sql1 =
        "select filesize from fileid_info where sender ='" + sender + "'and status = 'uploading'";
    std::vector<std::string> filesizeres = mysql_.selectmul(sql1.c_str());
    std::string sql2 =
        "select id from fileid_info where sender ='" + sender + "'and status = 'uploading'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql2.c_str());
    std::string sql3 =
        "select uploadedsize from fileid_info where sender ='" + sender + "'and status = 'uploading'";
    std::vector<std::string> uploadedres = mysql_.selectmul(sql3.c_str());
    std::vector<filestatusserver> res;
    for (int i = 0; i < filenameres.size();i++){
        filestatusserver ff;
        ff.filename=filenameres[i];
        ff.total=filesizeres[i];
        ff.id = fileidres[i];
        ff.sended = uploadedres[i];
        res.push_back(ff);
    }
    return res;
}
std::string FILEredis::getfilepath(std::string fileid){
    auto s = redis_.exists({"file:" + fileid});
    redis_.sync_commit();
    if (!s.get().as_integer()) {
        std::string sql =
            "select filepath from fileid_info where id ='" + fileid + "'";
        std::string filepath = mysql_.selectstring(sql.c_str());
        return filepath;
    }
    auto f=redis_.hget("file:"+fileid,"filepath");
    redis_.sync_commit();
    std::string filepath = f.get().as_string();
    return filepath;
}