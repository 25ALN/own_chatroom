#include "../common.hpp"

class login_re_zhuxiao{
    public:
    int check_account(redisContext* conn,const char* account,const char* password,const char *name);
    int check_login(redisContext* conn,const char* account,const char* password);
    int delete_account(redisContext* conn, const char* account,const char *password);
    int change_password(redisContext* conn, const char* account,const char* new_password);
};

int login_re_zhuxiao::check_account(redisContext* conn,const char* account,const char* password,const char *name){
    std::string key="user:"+std::string(account);
    // 检查账号是否已存在
    redisReply* check_reply = (redisReply*)redisCommand(conn,"EXISTS %s", account);
    if (check_reply->integer==1) {
        std::cout<<"账号"<<account<<"已存在，无法注册"<<std::endl;
        freeReplyObject(check_reply);
        return 0; // 注册失败
    }
    freeReplyObject(check_reply);
    // redisReply* reply = (redisReply*)redisCommand(conn, "SET %s %s", account, password);
    redisReply* reply = (redisReply*)redisCommand(conn, "HSET %s password %s name %s",key.c_str(),password,name);
    if(reply->type==REDIS_REPLY_INTEGER){
        std::cout<<"账号"<<account<<"注册成功"<<std::endl;
        freeReplyObject(reply);
        return 1; // 注册成功
    }else{
        std::cout<<"注册失败,未知错误"<<std::endl;
        freeReplyObject(reply);
        return 0;
    }
}

int login_re_zhuxiao::check_login(redisContext* conn,const char* account,const char* password){
    std::string key="user:"+std::string(account);
    redisReply* reply = (redisReply*)redisCommand(conn, "EXISTS %s",key.c_str());
    if (reply->integer==0){
        std::cout<<"账号"<<account<<"不存在"<<std::endl;
        freeReplyObject(reply);
        return 0;
    }
    freeReplyObject(reply);

    int result=0;
    redisReply* getpassword=(redisReply*)redisCommand(conn,"HGET %s password",key.c_str());
    if(strcmp(password,getpassword->str)==0){
        result=1;
    }
    if(!result){
        std::cout<<"密码错误"<<std::endl;
    }
    freeReplyObject(getpassword);
    return result;
}

int login_re_zhuxiao::delete_account(redisContext* conn, const char* account,const char *password){
    int result=0;
    std::string key="user:"+std::string(account);
    redisReply* chma=(redisReply*)redisCommand(conn,"HGET %s password",key.c_str());
    std::cout<<"key="<<key<<std::endl;
    if(chma->str==NULL){
        std::cout<<"no password"<<std::endl;
    }
    if(!strcmp(chma->str,password)==0){
        std::cout<<"密码错误，你无权注销该账号!"<<std::endl;
        freeReplyObject(chma);
        result=2;
        return result;
    }
    freeReplyObject(chma);
    redisReply* reply = (redisReply*)redisCommand(conn, "DEL %s",key.c_str());
    if (reply->type == REDIS_REPLY_INTEGER) {
        if (reply->integer==1){
            std::cout<<"账号"<<account<<"已成功注销"<<std::endl;
            result=1;
            std::string temp=(std::string)account;
            //删除好友
            std::string friend_key=temp+":friends";
            redisReply *delf=(redisReply *)redisCommand(conn,"DEL %s",friend_key.c_str());
            freeReplyObject(delf);
            //删除屏蔽列表
            std::string hmd_key=temp+":hmd_account";
            redisReply *hmd=(redisReply *)redisCommand(conn,"DEL %s",hmd_key.c_str());
            freeReplyObject(hmd);
            //删除加入的群
            std::string joined_key=temp+":joined_groups";
            redisReply *joined=(redisReply *)redisCommand(conn,"DEL %s",joined_key.c_str());
            freeReplyObject(joined);
        }else{
            std::cout<<"账号"<<account<<"不存在，无法注销"<<std::endl;
            result=0;
        }
    } else {
        std::cout<<"注销操作失败,未知错误"<<std::endl;
    }
    freeReplyObject(reply);
    return result;
}

int login_re_zhuxiao::change_password(redisContext* conn, const char* account,const char* new_password){
    redisReply* reply = (redisReply*)redisCommand(conn, "SET %s %s", account, new_password);
    if(reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0) {
        std::cout<<"账号"<<account<<"密码修改成功"<<std::endl;
        freeReplyObject(reply);
        return 1;
    }else{
        std::cout<<"密码修改失败,未知错误"<<std::endl;
        freeReplyObject(reply);
        return 0;
    }
}