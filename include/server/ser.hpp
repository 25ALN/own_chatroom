#ifndef SER_HPP
#define SER_HPP

#include "ftps.hpp"
#include "net_connect.hpp"
#include "friends.hpp"
#include "Groups_manage.hpp"

class chatserver {
public:
    chatserver(const std::string &ip, int port);
    void start();
    ~chatserver();
    
private:
    net_init network;
    friends friend_manager;
    GroupManager group_manager;
    redisContext *conn;

    std::unordered_map<int,clientmessage> clientm;
    std::mutex client_lock;
    int epoll_fd;
    int server_fd;
    std::string server_ip;
    int server_port;
    const int maxevent=1000;

    void heart_check();
};

#endif