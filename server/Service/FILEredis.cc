#include "FILEredis.h"
FILEredis::FILEredis() {
    std::string sql =
        "create table if not exists file_info (fileid int auto_increment "
        "primary key,sender varchar(100),reciver varchar(100),groupname "
        "varchar(300),filename varchar(100),filesize varchar(100),uppath "
        "varchar(300),ischat "
        "varchar(10),status varchar(30));";
    mysql_.exesql(sql.c_str());
    sql =
        "create table if not exists filestatus_info (transferid int "
        "auto_increment primary key,fileid int,sender varchar(100),curaccount varchar(100),status "
        "varchar(100),upsize varchar(300),downsize varchar(300),ischat varchar(10),downpath varchar(300),newfilename varchar(300));";
    mysql_.exesql(sql.c_str());
}
std::string FILEredis::friendupbegin(std::string sender,
                                     std::string reciver,
                                     std::string filename,
                                     std::string filesize,
                                     std::string ischat,
                                     std::string uppath) {
    std::string sql =
        "insert into file_info "
        "(sender,reciver,filename,filesize,ischat,uppath,status)values('" +
        sender + "','" + reciver + "','" + filename + "','" + filesize + "','" +
        ischat + "','" + uppath  + "','uping')";
    mysql_.exesql(sql.c_str());
    sql = "select last_insert_id()";
    std::string fileid = mysql_.selectstring(sql.c_str());
    sql =
        "insert into filestatus_info "
        "(fileid,sender,status,upsize,downsize,ischat)values('" +
        fileid + "','" + sender + "','uping','0','0','"+ischat+"')";
    mysql_.exesql(sql.c_str());
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
        "(sender,groupname,filename,filesize,ischat,uppath,status)values('" +
        sender + "','" + groupname + "','" + filename + "','" + filesize +
        "','" + ischat + "','" + uppath  + "','uping')";
    mysql_.exesql(sql.c_str());
    sql = "select last_insert_id()";
    std::string fileid = mysql_.selectstring(sql.c_str());
    sql =
        "insert into filestatus_info "
        "(fileid,sender,status,upsize,downsize,ischat)values('" +
        fileid + "','" + sender + "','uping','0','0','" + ischat + "')";
    mysql_.exesql(sql.c_str());
    return fileid;
}
void FILEredis::upfinish(std::string fileid) {
    std::string sql =
        "update filestatus_info set status = 'upfinish' where fileid = '" +
        fileid + "'";
    mysql_.exesql(sql.c_str());
    sql="update file_info set status = 'upfinish' where fileid = '"+fileid+"'";
    mysql_.exesql(sql.c_str());
}
void FILEredis::downfinish(std::string fileid) {
    std::string sql =
        "update filestatus_info set status = 'downfinish' where fileid = '" +
        fileid + "'";
    mysql_.exesql(sql.c_str());
    sql = "update filestatus_info set downsize = '0' where fileid = '" +
          fileid + "'";
    mysql_.exesql(sql.c_str());
}
void FILEredis::transferinsert(std::string fileid,
                               std::string account,
                               std::string downpath,std::string ischat,std::string newfilename) {
    if(newfilename.empty()){
        std::string sql =
            "insert into filestatus_info "
            "(fileid,curaccount,status,downsize,ischat,downpath)values('" +
            fileid + "','" + account + "','downing','0','" + ischat + "','" +
            downpath + "')";
        mysql_.exesql(sql.c_str());
    }else{
        std::string sql =
            "insert into filestatus_info "
            "(fileid,curaccount,status,downsize,ischat,downpath,newfilename)"
            "values('" +
            fileid + "','" + account + "','downing','0','" + ischat + "','" +
            downpath + "','" + newfilename + "')";
        mysql_.exesql(sql.c_str());
    }
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
std::string FILEredis::getdownpath(std::string fileid,std::string account) {
    std::string sql =
        "select downpath from filestatus_info where fileid = '" + fileid + "' and status = 'downing' and curaccount = '"+account+"'";
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
std::string FILEredis::getdownsize(std::string fileid,std::string account) {
    std::string sql =
        "select downsize from filestatus_info where fileid = '" + fileid + "' and curaccount = '"+account+"'";
    std::string downsize = mysql_.selectstring(sql.c_str());
    return downsize;
}
std::string FILEredis::getfilename(std::string fileid) {
    std::string sql =
        "select filename from file_info where fileid = '" + fileid + "'";
    std::string filename = mysql_.selectstring(sql.c_str());
    return filename;
}
std::vector<json> FILEredis::getdownfilelist(std::string account,std::string ischat){
    std::vector<json> res;
    std::string sql = "select fileid from file_info where reciver = '" +
                      account + "' and status = 'upfinish' and ischat = '"+ischat+"'";
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    std::string s;
    for (auto id : fileidres) {
        json jj;
        sql = "select filename from file_info where fileid = '" + id + "'";
        s=mysql_.selectstring(sql.c_str());
        jj["filename"] = s;
        sql = "select sender from file_info where fileid = '" + id + "'";
        s = mysql_.selectstring(sql.c_str());
        jj["from"] = s;
        jj["ID"] = id;
        res.push_back(jj);
    }
    return res;
}
std::vector<json> FILEredis::getgroupdownfilelist(std::vector<std::string> grouplist){
    std::vector<json> res;
    std::string sql;
    std::string fileid;
    std::string filename;
    std::string from;
    for (int i = 0; i < grouplist.size(); i++) {
        json jj;
        sql = "select fileid from file_info where groupname = '" +
              grouplist[i] + "' and status = 'upfinish' and ischat = '0'";
        std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
       for(auto fileid:fileidres) {
            sql = "select filename from file_info where fileid = '" + fileid +
                  "'";
            filename = mysql_.selectstring(sql.c_str());
            sql =
                "select sender from file_info where fileid = '" + fileid + "'";
            from = mysql_.selectstring(sql.c_str());
            jj["groupname"] = grouplist[i];
            jj["from"] = from;
            jj["filename"] = filename;
            jj["ID"] = fileid;
            res.push_back(jj);
        }
    }
    return res;
}
std::vector<json> FILEredis::getchatgroupdownfilelist(std::vector<std::string> grouplist){
    std::vector<json> res;
    std::string filename;
    std::string from;
    std::string fileid;
    std::string sql;
    for (int i = 0; i < grouplist.size(); i++) {
        json jj;
        sql = "select fileid from file_info where groupname = '" +
              grouplist[i] + "' and ischat ='1' and status = 'upfinish'";
        std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
        LOG_INFO << "fileid.size = " << fileidres.size()  ;
        for (auto fileid : fileidres) {
            sql =
                "select filename from file_info where fileid ='" + fileid + "'";
            filename = mysql_.selectstring(sql.c_str());
            jj["filename"] = filename;
            jj["from"] = grouplist[i];
            jj["ID"] = fileid;
            res.push_back(jj);
        }
    }
    return res;
}
std::string FILEredis::getnewfilename(std::string fileid,std::string account,std::string ischat){
    std::string sql="select newfilename from filestatus_info where fileid = '"+fileid+"' and curaccount ='"+account+"' and ischat ='"+ischat+"' and status = 'downing'";
    std::string newfilename = mysql_.selectstring(sql.c_str());
    return newfilename;
}
void FILEredis::deldownrecords(std::string ID,
                    std::string account,
                    std::string filepath,
                    std::string ischat,
                    std::string filename){
    std::string sql = "delete from filestatus_info where curaccount = '" +
                      account + "' and fileid = '" + ID + "' and filepath = '" +
                      filepath + "' and ischat = '" + ischat + "'";
    mysql_.exesql(sql.c_str());
}
std::vector<filestatusserver> FILEredis::getlist(std::string account,
                                                 std::string ischat,
                                                 std::string status,
                                                 bool group) {
    std::vector<filestatusserver> result;
    std::string sql;
    if (status == "uping") {
        sql = "select fileid from filestatus_info where sender = '" + account +
              "' and status = 'uping' and ischat = '" + ischat + "'";
    } else if (status == "downing") {
        sql = "select fileid from filestatus_info where curaccount = '" +
              account + "' and status = 'downing' and ischat = '" + ischat +
              "'";
    }
    std::vector<std::string> fileidres = mysql_.selectmul(sql.c_str());
    for (auto  id : fileidres) {
        sql = "select groupname from file_info where fileid = '" + id + "'";
        std::string groupname = mysql_.selectstring(sql.c_str());
        bool isgroupfile = !groupname.empty();
        if (isgroupfile != group) {
            continue;
        }
        filestatusserver file;
        file.fileid = id;
        if (!group) {
            if (status == "uping") {
                sql =
                    "select filename,reciver,filesize "
                    "from file_info where fileid = '" +
                    id + "'";
                auto row = mysql_.selectrow(sql.c_str());
                if (row.size() != 3) {
                    continue;
                }
                file.filename = row[0];
                file.reciver = row[1];
                file.filesize = row[2];
                sql =
                    "select upsize from filestatus_info "
                    "where fileid = '" +
                    id + "' and status = 'uping'";
                file.upsize = mysql_.selectstring(sql.c_str());
            } else {
                sql =
                    "select filename,sender,filesize "
                    "from file_info where fileid = '" +
                    id + "'";
                auto row = mysql_.selectrow(sql.c_str());
                if (row.size() != 3) {
                    continue;
                }
                file.filename = row[0];
                file.sender = row[1];
                file.filesize = row[2];
                sql =
                    "select downsize from filestatus_info "
                    "where fileid = '" +
                    id +
                    "' and status = 'downing' "
                    "and curaccount = '" +
                    account + "'";

                file.downsize = mysql_.selectstring(sql.c_str());
            }
        } else {
            sql =
                "select filename,groupname,filesize "
                "from file_info where fileid = '" +
                id + "'";
            auto row = mysql_.selectrow(sql.c_str());
            if (row.size() != 3) {
                continue;
            }
            file.filename = row[0];
            file.filesize = row[2];
            if (status == "uping") {
                file.reciver = row[1];
                sql =
                    "select upsize from filestatus_info "
                    "where fileid = '" +
                    id + "' and status = 'uping'";

                file.upsize = mysql_.selectstring(sql.c_str());
            } else {
                file.sender = row[1];
                sql =
                    "select downsize from filestatus_info "
                    "where fileid = '" +
                    id +
                    "' and status = 'downing' "
                    "and curaccount = '" +
                    account + "'";
                file.downsize = mysql_.selectstring(sql.c_str());
            }
        }
        result.push_back(file);
    }
    return result;
}