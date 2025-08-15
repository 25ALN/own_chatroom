#include "../ftpser.hpp"

int fsRecv(int fd,char *buf,int len,int flag);
int fsSend(int fd,char *buf,int len,int flag);
extern const int first_port;
extern int mes_travel_port;
extern const int maxevents;

class client_data{
    public:
    int client_fd;  //控制连接所用的描述符
    int listen_fd;   //listen描述符
    int data_fd;    //数据连接所用的描述符
    bool pasv_flag; //pasv模式是否打开的标志
    std::string server_ip;

    client_data(int fd,const std::string &ip):client_fd(fd),listen_fd(-1),data_fd(-1),pasv_flag(false)
    ,server_ip(ip){}
};
extern std::unordered_map<int,std::shared_ptr <client_data> > client_message; //利用哈希表来从将信息一一对应起来
class ftpserver{
    public:
    void start();
    ~ftpserver();

    private:
    
    int connect_init();
    void error_report(const std::string &x,int fd); //错误处理
    void ser_work(int fd,std::string ip);
    void clean_connect(int fd);

    void deal_new_connect(int ser_fd,int epoll_fd);
    void deal_pasv_data(int clinet_fd);
    void deal_STOR_data(std::shared_ptr<client_data> client,std::string filename);
    void deal_RETR_data(std::shared_ptr<client_data> client,std::string filename);
    void deal_client_data(int data_fd);
};

