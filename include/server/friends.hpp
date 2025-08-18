#ifndef FRIENDS_HPP
#define FRIENDS_HPP

#include "../common.hpp"
#include "threadpool.hpp"
#include "Groups_manage.hpp"


class friends {
public:
    friends(redisContext* conn, std::unordered_map<int, clientmessage>& clients);
    void deal_friends_part(int client_fd,std::string data);
    std::string deal_add_friends(int client_fd,std::string account);
    void deal_friends_add_hf(int client_fd, const std::string& response, const std::string& requester,std::string own_account);
    void delete_friends(int client_fd,std::string account);
    void chat_with_friends(int client_fd,std::string account,std::string data);
    void show_friends(int client_fd);
    void ingore_someone(int client_fd,std::string account);
    threadpool pool;
    
private:
    redisContext* conn;
    GroupManager gm;
    std::unordered_map<int, clientmessage>& clientm;
    std::deque<std::string> recv_buffer;
};

#endif