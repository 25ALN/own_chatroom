#ifndef NET_CONNECT_HPP
#define NET_CONNECT_HPP

#include "../common.hpp"
#include "friends.hpp"
#include "Groups_manage.hpp"

class net_init {
public:
    net_init(redisContext* conn, std::unordered_map<int, clientmessage>& clients, 
             const std::string& ip, int port);
    void connect_init();              
    void deal_new_connect();            
    void deal_client_mes(int client_fd);                     
    void find_password(int client_fd,std::string mes);
    void deal_login_in(int client_fd);
private:
    std::mutex client_lock;
    int epoll_fd;
    int server_fd;
    std::string server_ip;
    int server_port;
    redisContext *conn;
    std::unordered_map<int,clientmessage> clientm;
    std::deque<std::string> recv_buffer;
    friends manage;
    GroupManager gmaage;
    const int maxevent=1000;
};

#endif