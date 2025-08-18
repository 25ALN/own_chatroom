#include "../include/server/ser.hpp"


chatserver::chatserver(const std::string &ip, int port)
    : server_ip(ip),
      server_port(port),
      conn(nullptr),                          
      friend_manager(conn, clientm),          
      group_manager(conn, clientm),           
      network(conn, clientm, ip, port)        
{
    epoll_fd = -1;
    server_fd = -1;
}

void chatserver::start(){
    network.connect_init();

    std::thread file_travel([]{
        ftpserver a;
        a.start();
    });
    file_travel.detach();
    
    //心
    std::thread heart_check(&chatserver::heart_check,this);
    heart_check.detach();

    auto &x=clientm[server_fd];
    epoll_fd=epoll_create(1);
    struct epoll_event ev,events[maxevent];
    ev.data.fd=server_fd;
    ev.events=EPOLLET|EPOLLIN;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev)<0){
        perror("epoll_ctl");
        close(epoll_fd);
        close(server_fd);
    }
    while(true){
        int ready=epoll_wait(epoll_fd,events,maxevent,-1);
        for(int i=0;i<ready;i++){
            if(events[i].data.fd==server_fd){
                network.deal_new_connect();
            }else if(events[i].data.fd){
                network.deal_client_mes(events[i].data.fd);
            }
        }
    }
}

void chatserver::heart_check(){
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    //这个函数创建定时器文件描述符，用于定时进行发送心跳
    if (timer_fd < 0) {
        perror("timerfd_create");
        return;
    }

    itimerspec new_value;
    new_value.it_interval.tv_sec = 10;  // 每10秒触发一次
    new_value.it_interval.tv_nsec = 0;
    new_value.it_value.tv_sec = 10;     //最一开始的10秒后触发
    new_value.it_value.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &new_value, nullptr) < 0) {
        perror("timerfd_settime");
        close(timer_fd);
        return;
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = timer_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) < 0) {
        perror("epoll_ctl timer_fd");
        close(timer_fd);
        return;
    }

    uint64_t expirations;
    const time_t overtime = 15;  

    while (true) {

        int n = read(timer_fd, &expirations, sizeof(expirations));
        if (n != sizeof(expirations)) continue; 

        time_t now = time(nullptr);
        std::vector<int> to_remove;

        std::lock_guard<std::mutex> lock(client_lock); 
        for (auto &[fd, client] : clientm) {
            if (fd == server_fd) continue; 
            if (now - client.last_heart_time > overtime) {
                client.state = 0;
                client.login_step = 0;
                std::cout << "客户端" << fd << "心跳超时，关闭连接" << std::endl;
                to_remove.push_back(fd);
            }
        }

        for (int fd : to_remove) {
            close(fd);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
            clientm.erase(fd);
        }
    }

    close(timer_fd);
}

chatserver::~chatserver(){
    std::cout<<"开始关闭服务器"<<std::endl;
    for(auto[fd,clientmessage]:clientm){
        clientmessage.state=0;
        clientmessage.login_step=0;
        close(fd);
    }
    clientm.clear();
    if(server_fd!=-1){
        close(server_fd);
    }
    if(epoll_fd!=-1){
        close(epoll_fd);
    }
    std::cout<<"服务器已关闭"<<std::endl;
}