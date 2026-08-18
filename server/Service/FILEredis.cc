#include "FILEredis.h"
FILEredis::FILEredis() {
    std::string sql =
        "create table if not exists file_info (fileid int auto_increment "
        "primary key,sender varchar(100),reciver varchar(100),groupname "
        "varchar(300),filename varchar(100),filesize varchar(100),uppath "
        "varchar(300),ischat "
        "varchar(10));";
    mysql_.createinfo(sql.c_str());
    sql =
        "create table if not exists filestatus_info (transferid int "
        "auto_increment primary key,fileid int,sender varchar(100),curaccount varchar(100),status "
        "varchar(100),upsize varchar(300),downsize varchar(300),ischat varchar(10),downpath varchar(300));";
    mysql_.createinfo(sql.c_str());
}
std::string FILEredis::friendupbegin(std::string sender,
                                     std::string reciver,
                                     std::string filename,
                                     std::string filesize,
                                     std::string ischat,
                                     std::string uppath) {
    std::string sql =
        "insert into file_info "
        "(sender,reciver,filename,filesize,ischat,uppath)values('" +
        sender + "','" + reciver + "','" + filename + "','" + filesize + "','" +
        ischat + "','" + uppath  + "')";
    mysql_.addmsg(sql.c_str());
    sql = "select last_insert_id()";
    std::string fileid = mysql_.selectstring(sql.c_str());
    sql =
        "insert into filestatus_info "
        "(fileid,sender,status,upsize,downsize,ischat)values('" +
        fileid + "','" + sender + "','uping','0','0','"+ischat+"')";
    mysql_.addmsg(sql.c_str());
    return fileid;
}
std::string FILEredis::groupupbegin(std::string sender,
                                    std::string groupname,
                                    std::string filename,
                                    std::string filesize,
                                    std::string ischat,
                                    std::string uppath) {
    std::string sql =
        "insert into file_info "
        "(sender,groupname,filename,filesize,ischat,uppath)values('" +
        sender + "','" + groupname + "','" + filename + "','" + filesize +
        "','" + ischat + "','" + uppath  + "')";
    mysql_.addmsg(sql.c_str());
    sql = "select last_insert_id()";
    std::string fileid = mysql_.selectstring(sql.c_str());
    sql =
        "insert into filestatus_info "
        "(fileid,sender,status,upsize,downsize,ischat)values('" +
        fileid + "','" + sender + "','uping','0','0','" + ischat + "')";
    mysql_.addmsg(sql.c_str());
    return fileid;
}
void FILEredis::upfinish(std::string fileid) {
    std::string sql =
        "update filestatus_info set status = 'upfinish' where fileid = '" +
        fileid + "'";
    mysql_.changemsg(sql.c_str());
}
void FILEredis::downfinish(std::string fileid) {
    std::string sql =
        "update filestatus_info set status = 'downfinish' where fileid = '" +
        fileid + "'";
    mysql_.changemsg(sql.c_str());
}
void FILEredis::transferinsert(std::string fileid,
                               std::string account,
                               std::string downpath,std::string ischat) {
    std::string sql =
        "insert into filestatus_info "
        "(fileid,curaccount,status,downsize,ischat,downpath)values('" +
        fileid + "','" + account + "','downing','0','"+ischat+"','"+downpath+"')";
    mysql_.addmsg(sql.c_str());
}
std::string FILEredis::getsender(std::string fileid) {
    std::string sql =
        "select sender from file_info where fileid = '" + fileid + "'";
    std::string sender = mysql_.selectstring(sql.c_str());
    return sender;
}
std::string FILEredis::getreciver(std::string fileid){
    std::string sql =
        "select reciver from file_info where fileid = '" + fileid + "'";
    std::string reciver = mysql_.selectstring(sql.c_str());
    return reciver;
}
std::string FILEredis::getgroupname(std::string fileid){
    std::string sql =
        "select groupname from file_info where fileid = '" + fileid + "'";
    std::string groupname = mysql_.selectstring(sql.c_str());
    return groupname;
}
std::string FILEredis::getfilesize(std::string fileid) {
    std::string sql =
        "select filesize from file_info where fileid = '" + fileid + "'";
    std::string filesize = mysql_.selectstring(sql.c_str());
    return filesize;
}
std::string FILEredis::getdownpath(std::string fileid) {
    std::string sql =
        "select downpath from filestatus_info where fileid = '" + fileid + "' and status = 'downing'";
    std::string downpath = mysql_.selectstring(sql.c_str());
    return downpath;
}
std::string FILEredis::getuppath(std::string fileid) {
    std::string sql =
        "select uppath from file_info where fileid = '" + fileid + "'";
    std::string uppath = mysql_.selectstring(sql.c_str());
    return uppath;
}
std::string FILEredis::getupsize(std::string fileid) {
    std::string sql =
        "select upsize from filestatus_info where fileid = '" + fileid + "'";
    std::string upsize = mysql_.selectstring(sql.c_str());
    return upsize;
}
std::string FILEredis::getdownsize(std::string fileid) {
    std::string sql =
        "select downsize from filestatus_info where fileid = '" + fileid + "'";
    std::string downsize = mysql_.selectstring(sql.c_str());
    return downsize;
}
std::string FILEredis::getfilename(std::string fileid) {
    std::string sql =
        "select filename from file_info where fileid = '" + fileid + "'";
    std::string filename = mysql_.selectstring(sql.c_str());
    return filename;
}
std::vector<filestatusserver> FILEredis::getfriendupinglist(
    std::string account) {
    std::string sql =
        "select fileid from filestatus_info where sender = '"+ account +
        "' and status = 'uping' and ischat = '0'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> upsizeres;
    for (auto id : fileidres) {
        sql="select groupname from file_info where fileid = '"+id+"'";
        s=mysql_.selectstring(sql.c_str());
        if(s.empty()){
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);
            sql = "select reciver from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            senderres.push_back(s);
            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select upsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            upsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    if(senderres.size()!=0){
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.reciver = senderres[i];
            ff.upsize = upsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}
std::vector<filestatusserver> FILEredis::getchatfriendupinglist(
    std::string account) {
    std::vector<std::string> fileidres;
    std::string sql =
        "select fileid from filestatus_info where sender = '" + account +
        "' and ischat = '1' and status = 'uping'";
    fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> upsizeres;
    for (auto id : fileidres) {
        sql = "select groupname from file_info where fileid = '" + id + "'";
        s = mysql_.selectstring(sql.c_str());
        if(s.empty()){
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);
            sql = "select reciver from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            senderres.push_back(s);
            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select upsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            upsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    std::cout << "filename.size = " << filenameres.size() << std::endl;
    std::cout << "filesize.size = " << filesizeres.size() << std::endl;
    std::cout<<"fileid.size = "<<fileidres.size()<<std::endl;
    std::cout << "sender.size = " << senderres.size() << std::endl;
    std::cout << "upsize.size = " << upsizeres.size() << std::endl;
    if(!senderres.empty()){
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.reciver = senderres[i];
            ff.upsize = upsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}
std::vector<filestatusserver> FILEredis::getgroupupinglist(
    std::string account) {
    std::string sql =
        "select fileid from filestatus_info where  sender= '" + account +
        "' and status = 'uping' and ischat = '0'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> upsizeres;
    for (auto id : fileidres) {
        sql = "select groupname from file_info where fileid = '" + id + "'";
        s = mysql_.selectstring(sql.c_str());
        if(!s.empty()){
            senderres.push_back(s);
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);

            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select upsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            upsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    if(senderres.size()!=0){
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.reciver = senderres[i];
            ff.upsize = upsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}
std::vector<filestatusserver> FILEredis::getchatgroupupinglist(
    std::string account) {
    std::string sql =
        "select fileid from filestatus_info where sender = '" + account +
        "' and status = 'uping' and ischat = '1'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> upsizeres;
    for (auto id : fileidres) {
        sql = "select groupname from file_info where fileid = '" + id + "'";
        s = mysql_.selectstring(sql.c_str());
        if(!s.empty()){
            senderres.push_back(s);
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);
            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select upsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            upsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    std::cout << "filename.size = " << filenameres.size() << std::endl;
    std::cout << "filesize.size = " << filesizeres.size() << std::endl;
    std::cout << "fileid.size = " << fileidres.size() << std::endl;
    std::cout << "sender.size = " << senderres.size() << std::endl;
    std::cout << "upsize.size = " << upsizeres.size() << std::endl;
    if(senderres.size()!=0){
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.reciver = senderres[i];
            ff.upsize = upsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}
std::vector<filestatusserver> FILEredis::getfrienddowninglist(
    std::string account) {
    std::string sql =
        "select fileid from filestatus_info where curaccount = '" + account +
        "' and status = 'downing' and ischat = '0'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> upsizeres;
    for (auto id : fileidres) {
        sql="select groupname from file_info where fileid = '"+id+"'";
        s = mysql_.selectstring(sql.c_str());
        if(s.empty()){
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);
            sql = "select sender from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            senderres.push_back(s);
            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select downsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            upsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    if(senderres.size()!=0){
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.sender = senderres[i];
            ff.downsize = upsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}
std::vector<filestatusserver> FILEredis::getchatfrienddowninglist(
    std::string account) {
    std::vector<std::string> fileidres;
    std::string sql =
        "select fileid from filestatus_info where curaccount = '" + account +
        "' and ischat = '1' and status = 'downing'";
    fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> upsizeres;
    for (auto id : fileidres) {
        sql =
            "select groupname from file_info where fileid = '" + id + "'";
        s = mysql_.selectstring(sql.c_str());
        if(s.empty()){
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);
            sql = "select sender from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            senderres.push_back(s);
            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select downsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            upsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    if(!senderres.empty()){
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.sender = senderres[i];
            ff.downsize = upsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}
std::vector<filestatusserver> FILEredis::getgroupdowninglist(
    std::string account) {
    std::string sql =
        "select fileid from filestatus_info where curaccount = '" + account +
        "' and status = 'downing' and ischat = '0'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> downsizeres;
    for (auto id : fileidres) {
        sql = "select groupname from file_info where fileid = '" + id + "'";
        s = mysql_.selectstring(sql.c_str());
        if(!s.empty()){
            senderres.push_back(s);
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);
            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select downsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            downsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    if (senderres.size() != 0) {
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.sender = senderres[i];
            ff.downsize = downsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}
std::vector<filestatusserver> FILEredis::getchatgroupdowninglist(
    std::string account) {
    std::string sql =
        "select fileid from filestatus_info where curaccount = '" + account +
        "' and status = 'downing' and ischat = '1'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    std::vector<std::string> filenameres;
    std::vector<std::string> senderres;
    std::vector<std::string> filesizeres;
    std::vector<std::string> downsizeres;
    for (auto id : fileidres) {
        sql = "select groupname from file_info where fileid = '" + id + "'";
        s = mysql_.selectstring(sql.c_str());
        if(!s.empty()){
            senderres.push_back(s);
            sql = "select filename from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filenameres.push_back(s);
            sql = "select filesize from file_info where fileid = '" + id + "'";
            s = mysql_.selectstring(sql.c_str());
            filesizeres.push_back(s);
            sql = "select downsize from filestatus_info where fileid = '" + id +
                  "'";
            s = mysql_.selectstring(sql.c_str());
            downsizeres.push_back(s);
        }
    }
    std::vector<filestatusserver> res;
    if (senderres.size() != 0) {
        for (int i = 0; i < fileidres.size(); i++) {
            filestatusserver ff;
            ff.filename = filenameres[i];
            ff.filesize = filesizeres[i];
            ff.fileid = fileidres[i];
            ff.sender = senderres[i];
            ff.downsize = downsizeres[i];
            res.push_back(ff);
        }
    }
    return res;
}