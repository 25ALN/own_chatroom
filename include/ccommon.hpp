#ifndef CCOMMON_HPP
#define CCOMMON_HPP

#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <csignal>
#include <sys/epoll.h>
#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>
#include <set>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>
#include <sys/wait.h>
#include <netinet/in.h> //sockaddr_in, htons() 等
#include <arpa/inet.h> //inet_pton(), inet_ntoa() 等
#include <algorithm>
#include <sstream>
#include <sys/sendfile.h>
#include <hiredis/hiredis.h>
#include <netinet/tcp.h>  
#include <netinet/in.h>  
#include <unordered_set>
#include <regex>
#include <filesystem>
#include <termios.h>
#include <sys/timerfd.h>
#include "./client/ftpc.hpp"

struct cclientmessage{
    int data_fd;
    std::string ip;
    std::string own_account;
    int begin_chat_mark=0;
    int if_getchat_account=0;
    int if_online=0;
    int if_enter_group=0;
    int if_begin_group_chat=0;
    int reafy_return=0;
    int identify_pd=0;
    int ifhave_pb=-1;
    int identify_check=0;
    ftpclient fptc;
};

int Recv(int fd,char *buf,int len,int flag);
int Send(int fd, const char *buf, int len, int flags);
static bool recv_chatting=true;
static bool send_chatting=true;
static bool grecv_chat=true;
static bool gsend_chat=true;


#endif