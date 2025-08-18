#include "../include/client/Groups.hpp"

Groupsm::Groupsm(std::unordered_map<int, cclientmessage>& clients, 
                   const std::string& ip, 
                   int port)
    : clientmes(clients),           
      client_ip(ip),              
      client_port(port)           
{}

void Groupsm::groups(int client_fd){
    std::cout<<"        -----------------------------       "<<std::endl;
    std::cout<<"        请选择你的需求：               "<<std::endl;
    std::cout<<"        |      1.创建群聊             |      "<<std::endl;
    std::cout<<"        |      2.解散群聊             |      "<<std::endl;
    std::cout<<"        |      3.查看已加入的群聊     |      "<<std::endl;
    std::cout<<"        |      4.申请加群             |      "<<std::endl;
    std::cout<<"        |      5.加入群聊聊天         |      "<<std::endl;
    std::cout<<"        |      6.退出某群             |      "<<std::endl;
    std::cout<<"        |      7.查看群组成员         |      "<<std::endl;
    std::cout<<"        |      8.群主/管理员特权      |     "<<std::endl;
    std::cout<<"        |      9.拉人进群             |     "<<std::endl;
    std::cout<<"        |      10.返回                |      "<<std::endl;
    std::cout<<"        -----------------------------       "<<std::endl;
    int choose=0;
    int fail_times=0;
    while(true){
        std::cin>>choose;
        if(std::cin.fail()){
            std::cout<<"输入中包含无效字符请重新输入:";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n'); //这一步可以丢弃掉输入中的无效字符
            fail_times++;
            if(fail_times==3){
                std::cout<<"输入错误次数过多，请稍后重试"<<std::endl;
                return;
            }
            continue;
        }
        if(choose>=1&&choose<=10){
            break;
        }else{
            fail_times++;
            if(fail_times==3){
                std::cout<<"输入错误次数过多，请稍后重试"<<std::endl;
                return;
            }
            std::cout<<"输入格式错误请重新输入:";
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    if(choose==10){
        Send(client_fd,"10t",3,0);
        auto&client=clientmes[client_fd];
        client.if_enter_group=0;
        return;
    }
    switch (choose){
    case 1:
        create_group(client_fd,choose);
        break;
    case 2:
        disband_group(client_fd,choose);
        break;
    case 3:
        look_enter_group(client_fd,choose);
        break;
    case 4:
        apply_enter_group(client_fd,choose);
        break;
    case 5:
        groups_chat(client_fd,choose);
        break;
    case 6:
        quit_one_group(client_fd,choose);
        break;
    case 7:
        look_group_members(client_fd,choose);
        break;
    case 8:
        owner_charger_right(client_fd,choose);
        break;
    case 9:
        la_people_in_group(client_fd,choose);
        break;
    default:
        break;
    }
}

void Groupsm::create_group(int client_fd,int choose){
    std::string chose=std::to_string(choose);
    chose+=' ';
    std::cout<<"请输入你想创建的群聊的群号(9位):";
    long long group_number=group_number_get(group_number);
    if(group_number==0) return;
    int fail_times=0;
    std::string group_name;
    std::cout<<"请输入你群聊的名字:";
    std::regex alpha_only("^[A-Za-z]+$");
    while(true){
        if(fail_times==3){
            std::cout<<"输入错误次数过多，请稍后重试"<<std::endl;
            return;
        }
        std::getline(std::cin,group_name);
        if(group_name.size()>32){
            std::cout<<"群名过长,请重新输入:";
            fail_times++;
            continue;
        }
        if(!std::regex_match(group_name,alpha_only)){
            std::cout<<"名字不是纯英文字母,请重新输入:"<<std::endl;
            fail_times++;
            continue;
        }
        break;
    }
    std::string num=std::to_string(group_number);
    std::string all_message=num+group_name+"(group)";
    all_message.insert(0,chose);
    int n=Send(client_fd,all_message.c_str(),all_message.size(),0);
    if(n<0){
        perror("create send");
    }
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    groups(client_fd);
}

void Groupsm::disband_group(int client_fd,int choose){
    std::string chose=std::to_string(choose);
    chose+=' ';
    std::cout<<"请输入你想解散的群聊的群号(9位):";
    long long group_number=group_number_get(group_number);
    if(group_number==0) return;
    std::string all_message=std::to_string(group_number)+"(group)";
    all_message.insert(0,chose);
    int n=Send(client_fd,all_message.c_str(),all_message.size(),0);
    if(n<0){
        perror("disband send");
    }
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    groups(client_fd);
}

void Groupsm::look_enter_group(int client_fd,int choose){
    std::string chose=std::to_string(choose);
    chose+=' ';
    char buf[1024];
    memset(buf,'\0',sizeof(buf));
    std::string all_message=chose+"(group)";
    int n=Send(client_fd,all_message.c_str(),all_message.size(),0);
    if(n<0){
        perror("look enter send");
    }
    std::cout<<"   群ID      群名      群主ID"<<std::endl;
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    groups(client_fd);
}

void Groupsm::apply_enter_group(int client_fd,int choose){
    std::string chose=std::to_string(choose);
    chose+=' ';
    std::cout<<"请输入你想加入的群聊的群号(9位):";
    long long group_number=group_number_get(group_number);
    if(group_number==0) return;
    std::string all_message=std::to_string(group_number)+"(group)";
    all_message.insert(0,chose);
    Send(client_fd,all_message.c_str(),all_message.size(),0);

    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    groups(client_fd);
}

void Groupsm::groups_chat(int client_fd,int choose){
    auto&client=clientmes[client_fd];
    std::string chose=std::to_string(choose);
    chose+=' ';
    std::cout<<"请输入你想加入聊天的群聊账号:";
    long long group_number=group_number_get(group_number);
    if(group_number==0) return;
    std::string all_message=std::to_string(group_number)+"(group)";
    all_message.insert(0,chose);
    Send(client_fd,all_message.c_str(),all_message.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    if_finshmes=false;
    static int group_chatfd=-1;
    if(client.if_begin_group_chat==1&&group_chatfd==-1){
        std::cout<<"/gexit 可退出群聊"<<std::endl;
        std::cout<<"glist file列出可下载的文件"<<std::endl;
        std::cout<<"glook past查看群聊所有的历史记录"<<std::endl;
        std::cout<<"STOR + 文件 上传文件"<<std::endl;
        std::cout<<"RETR + 文件 下载文件"<<std::endl;
    }
    if(client.if_begin_group_chat==1){
        std::string gb_addr="239.";
        long long getaddr=group_number;
        gb_addr+=(std::to_string(getaddr%255)+'.');
        getaddr/=255;
        gb_addr+=(std::to_string(getaddr%255)+'.');
        getaddr/=255;
        gb_addr+=(std::to_string(getaddr%255));

        group_chatfd=socket(AF_INET,SOCK_DGRAM,0);
        int flag=fcntl(group_chatfd,F_GETFL,0);
        fcntl(group_chatfd,F_SETFL,flag|O_NONBLOCK);
        struct sockaddr_in addr;
        memset(&addr,0,sizeof(addr));
        addr.sin_family=AF_INET;
        addr.sin_port=htons(8888);
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
        
        int opt=1;
        int reuse_fd=setsockopt(group_chatfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int));

        if(reuse_fd<0){
            perror("reuse setsockopt");
            close(group_chatfd);
        }
        int bind_fd=bind(group_chatfd,(struct sockaddr*)&addr,sizeof(addr));
        if(bind_fd<0){
            perror("udp bind");
            close(group_chatfd);
        }
        //加入多播
        struct ip_mreq mreq;
        mreq.imr_interface.s_addr=htonl(INADDR_ANY);
        mreq.imr_multiaddr.s_addr=inet_addr(gb_addr.c_str());
        int set_fd=setsockopt(group_chatfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq));
        if(set_fd<0){
            perror("setsockopt");
            close(group_chatfd);
        }

        std::thread recv_thread([&]{
            while(grecv_chat){
                char gbuf[3000000];
                memset(gbuf,'\0',sizeof(gbuf));
                int n=Recv(group_chatfd,gbuf,sizeof(gbuf),0);
                if(n<0){
                    continue;
                }       
                gbuf[n]='\0';
                std::string a=gbuf;
                if(a.find("该群已被解散，不允许继续发送消息")!=std::string::npos){
                    std::cout<<a<<std::endl;
                    grecv_chat=false;
                    gsend_chat=false;
                    client.if_begin_group_chat=0;
                    break;
                }
                if(!a.empty()){
                    //检查消息是否确定到达
                    int pos1=a.find("$?");
                    int pos2=a.find("^!");
                    std::string tempac=a.substr(pos1+2,pos2-pos1-2);
                    if(tempac==client.own_account) {
                        continue;
                    }else{
                        while(!a.empty()){
                            int pos3=a.find("$?");
                            int expos=a.find("*g*(group)");
                            int pos4=a.find("^!");
                            std::string tempmes=a.substr(0,pos3);
                            if(expos!=-1){
                                tempmes=a.substr(0,expos);
                            }
                            a.erase(0,pos4+2);
                            if(!tempmes.empty()&&tempmes[0]!=0x02){
                                std::cout<<tempmes<<std::endl;
                            }
                        }
                    }
                }
            }
        });

        std::thread send_thread([&]{
            while(gsend_chat){
                std::string message;
                char tempbuf[4096];
                memset(tempbuf,'\0',sizeof(tempbuf));
                std::cin.getline(tempbuf,sizeof(tempbuf));
                message=tempbuf;
                if(gsend_chat==false){
                    break;
                }
                //加入特殊字符保证消息的到达
                message=message+"$?"+client.own_account+"^!";
                std::string sendmes=message;
                if(sendmes.substr(0,10)=="glook past"){
                    sendmes.insert(0,std::to_string(group_number));
                }
                std::string meslen=std::to_string(sendmes.size());
                meslen.insert(0,"ghead");
                meslen+=' ';
                sendmes.insert(0,meslen);
                Send(client_fd,sendmes.c_str(),sendmes.size(),0);
                if(message.substr(0,4)=="STOR"||message.substr(0,4)=="RETR"){
                    int pos=message.find("$?");
                    message=message.substr(0,pos);
                    int cpos=message.find("STOR ");
                    if(cpos!=-1){
                        std::string checkpath=message.substr(5,message.size()-5);
                        if (!std::filesystem::exists(checkpath)) {
                            std::cout<<"该路径不存在"<<std::endl;
                            continue;
                        }else if (std::filesystem::is_directory(checkpath)) {
                            std::cout<<"这是一个目录,不允许上传一个目录"<<std::endl;
                            continue;
                        }
                    }
                    message.insert(0,"gfi");
                    client.fptc.start(message,client.ip);
                }
                if(message.substr(0,6)=="/gexit"){
                    
                    gsend_chat=false;
                    grecv_chat=false;
                    client.if_begin_group_chat=0;
                    break;
                }
                message.clear();
                if(gsend_chat==false) break;
                sendmes.clear();
                message.clear();
            }
        });

        if(recv_thread.joinable()){
            recv_thread.join();
        }
        if(send_thread.joinable()){
            send_thread.join();
            client.if_begin_group_chat=0;
        }
        if(gsend_chat==false&&grecv_chat==false){
            //离开多播组
            set_fd=setsockopt(group_chatfd,IPPROTO_IP,IP_DROP_MEMBERSHIP,&mreq,sizeof(mreq));
            if(set_fd<0){
                perror("quit setsockopt");
                close(group_chatfd);
            }
            std::cout<<"已成功退出群聊聊天"<<std::endl;
            group_chatfd=-1;
            client.if_begin_group_chat=0;
            gsend_chat=true;
            grecv_chat=true;
        }
    }else{
        client.if_enter_group=0;
        return;
    }
}

void Groupsm::quit_one_group(int client_fd,int choose){
    std::string chose=std::to_string(choose);
    chose+=' ';
    std::cout<<"请输入你想退出的群聊的群号(9位):";
    long long group_number=group_number_get(group_number);
    if(group_number==0) return;
    std::string all_message=std::to_string(group_number)+"(group)";
    all_message.insert(0,chose);
    Send(client_fd,all_message.c_str(),all_message.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    groups(client_fd);
}

void Groupsm::look_group_members(int client_fd,int choose){
    std::string chose=std::to_string(choose);
    chose+=' ';
    std::cout<<"请输入你想查看群成员的群聊的群号(9位):";
    long long group_number=group_number_get(group_number);
    if(group_number==0) return;
    std::string all_message=std::to_string(group_number)+"(group)";
    all_message.insert(0,chose);
    Send(client_fd,all_message.c_str(),all_message.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    groups(client_fd);
}

void Groupsm::owner_charger_right(int client_fd,int choose){
    auto&client=clientmes[client_fd];
    std::string chose=std::to_string(choose);
    chose+=' ';
    std::string all_message=chose+"(group)";
    int n=Send(client_fd,all_message.c_str(),all_message.size(),0);
    if(n<0){
        perror("right send");
    }
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;

    if(client.identify_pd==1||client.identify_pd==0){
        std::cout<<"你还不是任何群的群主或者管理员,无权继续操作"<<std::endl;
        client.identify_pd=0;
        return;
    }
    std::cout<<"        请选择你的需求：               "<<std::endl;
    std::cout<<"        |      1.将某人移除群聊       |      "<<std::endl;
    if(client.identify_pd==3){
    std::cout<<"        |      2.添加管理员           |      "<<std::endl;
    std::cout<<"        |      3.移除管理员           |      "<<std::endl;  
    }
    std::cout<<"        |      4.返回                 |      "<<std::endl;
    chose.clear();
    if(client.identify_pd==3){
        std::getline(std::cin,chose);
        int failtime=0;
        while(chose.size()!=1||(chose!="1"&&chose!="2"&&chose!="3"&&chose!="4")){
            if(failtime==3){
                std::cout<<"输入错误次数过多请稍后重试！"<<std::endl;
                return;
            }
            std::cout<<"输入格式有误，请重新输入:";
            failtime++;
            std::getline(std::cin,chose);
        }
    }else{
        std::getline(std::cin,chose);
        int failtime=0;
        while(chose.size()!=1||(chose!="1"&&chose!="4")){
            if(failtime==3){
                std::cout<<"输入错误次数过多请稍后重试！"<<std::endl;
                return;
            }
            std::cout<<"输入格式有误，请重新输入:";
            failtime++;
            std::getline(std::cin,chose);
        }
    }
    if(chose=="4"){
        client.identify_pd=0;
        client.if_enter_group=0;
        return;
    }
    std::cout<<"请输入你想操作的群聊的群号(9位):";
    long long group_number=group_number_get(group_number);
    std::string temp=std::to_string(group_number);
    if(group_number==0) return;
    switch (chose[0]){
        case '1':
            move_someone_outgroup(client_fd,chose,temp);
            break;
        case '2':
        case '3':
            add_group_charger(client_fd,chose,temp);
            break;
        default:
            break;
    }
    client.identify_pd=0;
    groups(client_fd);
}

void Groupsm::la_people_in_group(int client_fd,int choose){
    auto&client=clientmes[client_fd];
    std::string account;
    std::cout<<"请输入你想要邀请的用户的账号(6位):";
    std::getline(std::cin,account);
    int failtimes=0;
    while(account.size()!=6||!std::all_of(account.begin(),account.end(),::isdigit)){
        if(failtimes==3){
            std::cout<<"错误次数过多请稍后重试"<<std::endl;
            return;
        }
        std::cout<<"账号格式不规范请重新输入:";
        failtimes++;
        std::getline(std::cin,account);
    }
    std::string group_number;
    std::cout<<"请输入你想要拉该用户进去的群号:";
    long long temp=group_number_get(temp);
    group_number=std::to_string(temp);
    std::string message=std::to_string(choose)+" "+account+"^^"+group_number+"(group)";
    Send(client_fd,message.c_str(),message.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    groups(client_fd);
}

void Groupsm::move_someone_outgroup(int client_fd,std::string xz,std::string group_number){
    auto&client=clientmes[client_fd];
    std::string account;
    std::cout<<"请输入你想要移除的群成员账号(6位):";
    std::getline(std::cin,account);
    int failtimes=0;
    while(account.size()!=6||!std::all_of(account.begin(),account.end(),::isdigit)){
        if(failtimes==3){
            std::cout<<"错误次数过多请稍后重试"<<std::endl;
            break;
        }
        std::cout<<"账号格式不规范请重新输入:";
        failtimes++;
        std::getline(std::cin,account);
    }
    xz+=' ';
    account.insert(0,xz);
    group_number.insert(0,"*");
    group_number+="(group)(group owner)";
    account+=group_number;
    Send(client_fd,account.c_str(),account.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
}

void Groupsm::add_group_charger(int client_fd,std::string xz,std::string group_number){
    auto&client=clientmes[client_fd];
    std::string account;
    std::cout<<"请输入你想要添加/移除管理员的群成员账号:";
    std::getline(std::cin,account);
    int failtimes=0;
    while(account.size()!=6||!std::all_of(account.begin(),account.end(),::isdigit)){
        if(failtimes==3){
            std::cout<<"错误次数过多请稍后重试"<<std::endl;
            return;
        }
        std::cout<<"账号格式不规范请重新输入:";
        failtimes++;
        std::getline(std::cin,account);
    }
    xz+=' ';
    account.insert(0,xz);
    group_number.insert(0,"*");
    group_number+="(group)(group owner)";
    account+=group_number;
    Send(client_fd,account.c_str(),account.size(),0);
    char buf[100];
    memset(buf,'\0',sizeof(buf));
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
}

long long Groupsm::group_number_get(long long a){
    int fail_times=0;
    while(true){
        std::cin>>a;
        if(std::cin.fail()){
            std::cout<<"输入中包含无效字符请重新输入:";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n'); //这一步可以丢弃掉输入中的无效字符
            fail_times++;
            if(fail_times==3){
                std::cout<<"输入错误次数过多，请稍后重试"<<std::endl;
                return 0;
            }
            continue;
        }
        if(a>99999999&&a<=999999999){
            break;
        }else{
            fail_times++;
            if(fail_times==3){
                std::cout<<"输入错误次数过多，请稍后重试"<<std::endl;
                return 0;
            }
            std::cout<<"群聊格式错误请重新输入:";
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    return a;
}
