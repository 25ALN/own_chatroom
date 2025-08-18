#ifndef GROUPS_MANAGE_HPP
#define GROUPS_MANAGE_HPP

#include "../common.hpp"

class GroupManager{
public:
    GroupManager(redisContext* conn, std::unordered_map<int, clientmessage>& clients);
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


private:
    redisContext* conn;
    std::deque<std::string> recv_buffer;
    std::unordered_map<int, clientmessage>& clientm;
};

#endif