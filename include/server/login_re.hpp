#include "../common.hpp"

class login_re_zhuxiao{
    public:
    int check_account(redisContext* conn,const char* account,const char* password,const char *name);
    int check_login(redisContext* conn,const char* account,const char* password);
    int delete_account(redisContext* conn, const char* account,const char *password);
    int change_password(redisContext* conn, const char* account,const char* new_password);
};

