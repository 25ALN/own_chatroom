#include "../common.hpp"
#include "ftpc.hpp"
static bool recv_chatting=true;
static bool send_chatting=true;
int Send(int fd, const char *buf, int len, int flags);
int Recv(int fd,char *buf,int len,int flag);
static bool grecv_chat=true;
static bool gsend_chat=true;

struct clientmessage{
    int data_fd;
    std::string ip;
    std::string own_account;
    int begin_chat_mark=0;
    int if_getchat_account=0;
    int if_online=0;
    int if_enter_group=0;
    int if_begin_group_chat=0;
    int reafy_return=0;
    int identify_pd=0;
    int ifhave_pb=-1;
    int identify_check=0;
    ftpclient fptc;
};

class chatclient{
    public:
    chatclient(const std::string &tempip,int tempport);
    void start();
    ~chatclient();
    private:

    void connect_init();
    void caidan();
    void deal_login_mu(char *a);
    void find_password(std::string chose);
    void deal_else_gn();

    void delete_friends(int client_fd);
    void show_friends(int client_fd);
    void chat_with_friends(int client_fd,std::string request);
    void add_friends(int client_fd,std::string buf);
    void ingore_someone(int client_fd);
    void deal_new_message(int client_fd,std::string message);
    void new_message_show(int client_fd);
    void groups(int client_fd);
    
    void flush_recv_buffer(int sockfd);
    std::string check_set(std::string temp);

    int check_if_newmes(int client_fd);
    //心
    void heart_check();
    void heart_work_thread();

    void create_group(int client_fd,int choose);
    void disband_group(int client_fd,int choose); 
    void look_enter_group(int client_fd,int choose);
    void apply_enter_group(int client_fd,int choose);
    void groups_chat(int client_fd,int choose);
    void quit_one_group(int client_fd,int choose);
    void look_group_members(int client_fd,int choose);
    
    void owner_charger_right(int client_fd,int choose);
    void move_someone_outgroup(int client_fd,std::string xz,std::string group_number);
    void add_group_charger(int client_fd,std::string xz,std::string group_number);

    void la_people_in_group(int client_fd,int choose);
    void deal_apply_mes(int client_fd,std::string message);

    long long group_number_get(long long a);

    bool if_login=false;
    bool if_finshmes=false;
    //心
    std::atomic<bool> heart_if_running{false};
    std::thread heart_beat;
    std::atomic<time_t> last_heart_time{0};

    int client_fd;
    std::unordered_map<int,clientmessage> clientmes;
    
    std::set<std::string> recv_message;  
    std::string client_ip;
    int client_port;
    std::atomic<bool>mesmark=false;
};