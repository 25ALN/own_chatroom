#include "../include/client/cli.hpp"

chatclient::chatclient(const std::string &tempip,int tempport):
    client_ip(tempip),
    client_port(tempport),
    init(clientmes,tempip,tempport),
    f(clientmes,tempip,tempport),
    manage(clientmes,tempip,tempport)
{}

void chatclient::start(){
    init.connect_init();

    while(true){
        if(!if_login){
            init.caidan();
        }else{
            f.deal_else_gn();
        }
    }
}

chatclient::~chatclient(){
    std::cout<<"开始关闭该客户端"<<std::endl;

    //心
    heart_if_running=false;
    if(heart_beat.joinable()){
        heart_beat.join();
    }

    for(auto[fd,clientmesage]:clientmes){
        close(fd);
    }
    if(client_fd!=-1){
        close(client_fd);
    }
    std::cout<<"该客户端已成功关闭！"<<std::endl;
}

