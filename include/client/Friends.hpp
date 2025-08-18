#ifndef FRIENDS_HPP
#define FRIENDS_HPP

#include "../ccommon.hpp"
#include "Groups.hpp"
#include "ftpc.hpp"

class Friends{
    public:
    Friends(std::unordered_map<int, cclientmessage>& clients,const std::string& ip, int port);
    
    void deal_else_gn();

    void delete_friends(int client_fd);
    void show_friends(int client_fd);
    void chat_with_friends(int client_fd,std::string request);
    void add_friends(int client_fd,std::string buf);
    void ingore_someone(int client_fd);
    void deal_new_message(int client_fd,std::string message);
    void new_message_show(int client_fd);
    void flush_recv_buffer(int sockfd);
    void deal_apply_mes(int client_fd,std::string message);
    std::string check_set(std::string temp);
    private:
    Groupsm manage;
    bool if_login=false;
    bool if_finshmes=false;
    
    int client_fd;
    std::unordered_map<int,cclientmessage> clientmes;

    std::set<std::string> recv_message;  
    std::string client_ip;
    int client_port;
    std::atomic<bool>mesmark=false;
};

#endif