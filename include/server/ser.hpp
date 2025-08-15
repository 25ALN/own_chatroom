#include "../common.hpp"
#include "login_re.hpp"
#include "ftps.hpp"
#include "threadpool.hpp"

int Recv(int fd,char *buf,int len,int flag);
int Send(int fd, const char *buf, int len, int flags);
struct clientmessage{
    std::string ip;
    int port;
    int data_fd;
    int state=0;
    int login_step=0;
    std::string cur_user;
    std::string mark;

    std::string zh;
    std::string mm;
    std::string name;

    int if_begin_chat=0;
    int if_enter_group=0;
    int if_check_identity=0;
    int if_charger=0;
    int if_begin_group_chat=0;
    std::string chat_with;
    std::string group_chat_num;
    std::string group_message;

    time_t last_heart_time;
    std::string request;
    char new_choose;
    
};

class chatserver{
    public:
    chatserver(const std::string &ip,int port);
    void start();
    ~chatserver();
    private:
    std::mutex client_lock;
    threadpool pool;
    void connect_init();
    void deal_new_connect();
    void deal_client_mes(int client_fd);
    void deal_epoll_event();
    void deal_friends_part(int client_fd,std::string data);
    void deal_login_in(int client_fd);
    void find_password(int client_fd,std::string mes);
    std::string deal_add_friends(int client_fd,std::string account);
    void deal_friends_add_hf(int client_fd, const std::string& response, const std::string& requester,std::string own_account);
    void delete_friends(int client_fd,std::string account);
    void chat_with_friends(int client_fd,std::string account,std::string data);
    void show_friends(int client_fd);
    void ingore_someone(int client_fd,std::string account);

    int send_group_apply_mess(int client_fd,std::string mes);

    void groups(int client_fd);
    void create_groups(int client_fd);
    void disband_groups(int client_fd);

    void apply_enter_group(int client_fd);
    void check_identify(int client_fd,std::string mes);
    void add_user_enterg(int client_fd,std::string mes);

    void groups_chat(int client_fd);
    void look_enter_groups(int client_fd);
    void quit_one_group(int client_fd);
    void look_group_members(int client_fd);
    void own_charger_right(int client_fd);
    void la_people_in_group(int client_fd);
    void agree_enter_group(int client_fd);
    int check_acount_ifexists(int client_fd,std::string acout,std::string group_num);

    //test
    void save_group_application(const std::string& group_number, const std::string& applicant);

    void heart_check();
    redisContext *conn;
    std::string server_ip;
    int server_port;
    int epoll_fd;
    int server_fd;
    std::unordered_map<int,clientmessage> clientm;
    std::deque<std::string> recv_buffer;
};
const int maxevent=1000;