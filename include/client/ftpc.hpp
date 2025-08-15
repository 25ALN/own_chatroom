#include "../ftpcli.hpp"

int fcRecv(int fd,char *buf,int len,int flags);
int fcSend(int fd,const char *buf,int len,int flags);
void error_report(const std::string &x,int fd);
static int mark=1;
class ftpclient{
    public:
    void start(std::string message,std::string serip);
    ~ftpclient();
    
    private:

    const int first_port=2100;
    int data_port=-1;
    std::string IP;
    const int maxt=1000;
    int client_fd=-1;

    void connect_init();
    void start_PASV_mode(int fd,std::string first_m);
    void get_ip_port(std::string ser_mesage);
    void deal_willsend_message(int fd,char m[1024]);
    void deal_send_message(int fd,std::string m);
    void deal_get_file(std::string filename,int fd);
    void deal_up_file(std::string filename,int fd);
};