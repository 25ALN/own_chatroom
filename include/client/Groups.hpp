#ifndef GROUPS_HPP
#define GROUPS_HPP

#include "../ccommon.hpp"
#include "ftpc.hpp"

class Groupsm{
    public:
    Groupsm(std::unordered_map<int, cclientmessage>& clients,const std::string& ip, int port);

    void groups(int client_fd);
    int check_if_newmes(int client_fd);
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
    

    long long group_number_get(long long a);

    private:
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