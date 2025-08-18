#ifndef NCONNECT_HPP
#define NCONNECT_HPP

#include "../ccommon.hpp"

class Netconnect{
    public:
    Netconnect(std::unordered_map<int, cclientmessage>& clients, 
             const std::string& ip, int port);
    void connect_init();
    void caidan();
    void deal_login_mu(char *a);
    void find_password(std::string chose);
    void heart_check();
    void heart_work_thread();
    
    private:
    bool if_login=false;
    bool if_finshmes=false;
    //心
    std::atomic<bool> heart_if_running{false};
    std::thread heart_beat;
    std::atomic<time_t> last_heart_time{0};

    int client_fd;
    std::unordered_map<int,cclientmessage> clientmes;

    std::set<std::string> recv_message;  
    std::string client_ip;
    int client_port;
    std::atomic<bool>mesmark=false;
};


#endif