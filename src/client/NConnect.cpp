#include "../include/client/NConnect.hpp"

Netconnect::Netconnect(std::unordered_map<int, cclientmessage>& clients, 
                   const std::string& ip, 
                   int port)
    : clientmes(clients),           
      client_ip(ip),              
      client_port(port)           
{}

void Netconnect::heart_work_thread(){
    const char heart_cmd=0x05; //表示一个字节的大小
    const int heart_time=3;
    while(heart_if_running){
        if(Send(client_fd,&heart_cmd,1,0)<=0){
            perror("发送心跳失败");
            break;
        }
        last_heart_time=time(nullptr);
        sleep(heart_time);
    }

    std::cout<<"发送心跳终止，连接断开"<<std::endl;
    close(client_fd);
    client_fd=-1;
    if_login=false;
    exit(1);
}

void Netconnect::heart_check(){
    int alive=1;
    if(setsockopt(client_fd,SOL_SOCKET,SO_KEEPALIVE,&alive,sizeof(alive))<0){
        perror("setsockopt alive");
    }
    int idle=30;
    if(setsockopt(client_fd,SOL_TCP,TCP_KEEPIDLE,&idle,sizeof(idle))<0){
        perror("setsockopt idle");
    }
    int intv=5;
    if(setsockopt(client_fd,SOL_TCP,TCP_KEEPINTVL,&intv,sizeof(intv))<0){
        perror("setsockopt intv");
    }
    int cnt=3;
    if(setsockopt(client_fd,SOL_TCP,TCP_KEEPCNT,&cnt,sizeof(cnt))<0){
        perror("setsockopt cnt");
    }
}

void Netconnect::connect_init(){
    std::cout<<"开始连接服务端"<<std::endl;

    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_cc[VEOF] = _POSIX_VDISABLE; // 禁用 EOF
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    client_fd=socket(AF_INET,SOCK_STREAM,0);
    if(client_fd<0){
        perror("socket");
        close(client_fd);
    }    
    struct sockaddr_in clmes;
    clmes.sin_family=AF_INET;
    clmes.sin_port=htons(client_port);
    inet_pton(AF_INET,client_ip.c_str(),&clmes.sin_addr);//小组网络
    //inet_pton(AF_INET,"192.168.110.68",&clmes.sin_addr);//寝室网络
    //inet_pton(AF_INET,"192.168.20.143",&clmes.sin_addr);//手机热点
    int len=sizeof(clmes);
    int connect_fd=connect(client_fd,(struct sockaddr*)&clmes,len);
    if(connect_fd<0){
        perror("connect");
        close(client_fd);
        return;
    }
    int flag=fcntl(client_fd,F_GETFL,0);
    fcntl(client_fd,F_SETFL,flag|O_NONBLOCK);

    clientmes[client_fd].data_fd=-1;
    std::cout<<"已成功连接到服务端!"<<std::endl;

    //心
    heart_if_running=true;
    heart_beat=std::thread(&Netconnect::heart_work_thread,this);
    heart_beat.detach();
}

void Netconnect::caidan(){
    std::cout<<"        ---------------------       "<<std::endl;
    std::cout<<"        |      聊天室        |      "<<std::endl;
    std::cout<<"        |      1.登陆        |      "<<std::endl;
    std::cout<<"        |      2.注册        |      "<<std::endl;
    std::cout<<"        |      3.注销        |      "<<std::endl;
    std::cout<<"        |      4.退出        |      "<<std::endl;
    std::cout<<"        |      5.找回密码    |       "<<std::endl;
    std::cout<<"        ---------------------       "<<std::endl;
    std::thread allrecv([&]{
        auto&client=clientmes[client_fd];
        while(true){
            static int stopmark=0;
            if(stopmark==0){
                static std::mutex recv_lock;
                std::unique_lock<std::mutex> x(recv_lock);
                char buf[3000000];
                memset(buf,'\0',sizeof(buf));
                int n=Recv(client_fd,buf,sizeof(buf),0);
                if(n==-1){
                    std::cout<<"服务器已关闭，客户端即将退出"<<std::endl;
                    close(client_fd);
                    client_fd=-1;
                    if_login=false;
                    break;
                }
                if(n==0) continue;
                std::string temp=buf;
                if(temp.find(0x06)!=std::string::npos){
                    for(int i=0;i<temp.size();i++){
                        if(static_cast<int>(temp[i])==6||temp[i]=='\n'||temp[i]==' '){
                            temp.erase(i,1);
                        }
                    }
                }
                if(temp[0]==0x06||temp.size()<=0){
                    continue;
                }
                if((temp.find("[好友请求")!=std::string::npos&&temp.find(client.own_account)!=std::string::npos)
                ||temp.find("请求加入群聊")!=std::string::npos){
                    int pos=temp.find("(");
                    temp=temp.substr(0,pos);
                    recv_message.insert(temp);
                    temp.erase();
                    stopmark=1;
                }else{
                    if(temp.find("(login success)")!=std::string::npos&&temp.find("成功")!=std::string::npos){
                        if_login=true;
                        temp.erase(temp.find("(login success)"),16);
                        if_finshmes=true;
                    }
                    if(temp.find("可以向该好友")!=std::string::npos){
                        send_chatting=true;
                        if(temp.find("在线")!=std::string::npos){
                            client.if_online=1;
                        }else{
                            client.if_online=0;
                        }
                        client.if_getchat_account=1;
                    }else if(temp.find("no own chat")!=std::string::npos&&temp.size()<80){
                        temp.erase(temp.find("(no own chat"),14);
                        client.reafy_return=1;
                    }
               
                    if(temp.find(client.own_account)!=std::string::npos&&temp.find("0x01")!=std::string::npos){
                        if(temp.find("无法继续发送")!=std::string::npos){
                            send_chatting=false;
                            client.if_getchat_account=0;
                            client.begin_chat_mark=0;
                        }
                        while(!temp.empty()){
                            int pos2=temp.find(")0x01");
                            if(pos2==-1||pos2-7>temp.size()) {
                                temp.clear();
                                break;
                            }
                            std::string ownmes=temp.substr(0,pos2-7);
                            std::cout<<ownmes<<std::endl;
                            temp.erase(0,pos2+5);
                        }
                        continue;
                    }
                    if(temp.find('[')!=std::string::npos&&temp.find("](")!=std::string::npos&&temp.find(')')-temp.find('(')==7){
                        temp=temp.substr(0,temp.find('('));
                        temp+=0x07;
                    }
                    if(temp=="find"||temp=="if find"){
                        if(temp=="find"){
                            client.identify_check=1;
                        }else{
                            client.identify_check=0;
                        }
                    }
                    if(temp.find("(look enter group)")!=std::string::npos){
                        int pos=temp.find("(look enter group)");
                        temp=temp.substr(0,pos);
                    }
                    if(temp.find("(pblist)")!=std::string::npos&&temp.size()<200){
                        if(temp.find("当前还没有用户被你屏蔽")!=std::string::npos){
                            client.ifhave_pb=0;
                        }else{
                            std::cout<<"已屏蔽的好友:"<<std::endl;
                            client.ifhave_pb=1;
                        }
                        temp=temp.substr(0,temp.find("(pblist)"));
                    }
                    if(temp.find("可以在该群中发送消息了")!=std::string::npos&&temp.find("group chat begin")!=std::string::npos){
                        temp=temp.substr(0,temp.find("(group chat begin)"));
                        client.if_begin_group_chat=1;
                    }
                    if(temp.find("(identify)")!=std::string::npos){
                        temp=temp.substr(0,temp.find("(identify)"));
                        if(temp.find("成员")!=std::string::npos){
                            client.identify_pd=1;
                        }else if(temp.find("管理员")!=std::string::npos){
                            client.identify_pd=2;
                        }else if(temp.find("群主")!=std::string::npos){
                            client.identify_pd=3;
                        }
                        if_finshmes=true;
                        continue;
                    }
                    std::cout<<temp<<std::endl;
                    if(temp.find(0x07)==std::string::npos){
                        if_finshmes=true;
                    }
                    temp.clear();
                    stopmark=0;
                    continue;
                }
                x.unlock();
                stopmark=0;
            }
        }
    });
    allrecv.detach();
    if(client_fd==-1) {
        exit(0);
    }
    char choose[10];
    memset(choose,'\0',sizeof(10));
    int tempc=0;
    int fail_times=0;
    while(true){
        std::cin>>tempc;
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
        if(tempc>=1&&tempc<=5){
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
    std::string temp=std::to_string(tempc);
    strcpy(choose,temp.data());
    strncat(choose," choose",sizeof(" choose"));
    if(choose[0]=='4'){
        std::cout<<"已退出客户端"<<std::endl;
        exit(0);
    }
    if(choose[0]=='5'){
        find_password(choose);
        return;
    }else{
        Send(client_fd,choose,strlen(choose),0);
    }
    deal_login_mu(choose);
}

void Netconnect::find_password(std::string chose){
    std::cout<<"请输入你想要找回密码的账号:";
    std::string account;
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
    std::cout<<"请输入你的qq号:";
    std::string email;
    std::getline(std::cin,email);
    failtimes=0;
    while(email.size()!=10||!std::all_of(account.begin(),account.end(),::isdigit)){
        if(failtimes==3){
            std::cout<<"错误次数过多请稍后重试"<<std::endl;
            return;
        }
        std::cout<<"账号格式不规范请重新输入:";
        failtimes++;
        std::getline(std::cin,account);
    }
    email+="@qq.com";
    std::string allmes="5 "+account+" "+email;
    int n=Send(client_fd,allmes.c_str(),allmes.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    std::cout<<"请输入验证码:";
    std::string check_mes;
    std::getline(std::cin,check_mes);
    check_mes.insert(0,"yzm check mes");
    Send(client_fd,check_mes.c_str(),check_mes.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
}

void Netconnect::deal_login_mu(char *a){
    auto &x=clientmes[client_fd];
    x.ip=client_ip;
    char account[1024],name[100];
    char *password=nullptr;
    memset(account,'\0',sizeof(account));
    memset(name,'\0',sizeof(name));
    std::cout<<"请输入账号(6位):";
    std::cin.getline(account,sizeof(account));
    std::string actemp=account;
    int failtimes=0;
    while(actemp.size()!=6||!std::all_of(actemp.begin(),actemp.end(),::isdigit)){
        if(failtimes==3){
            std::cout<<"错误次数过多请稍后重试"<<std::endl;
            return;
        }
        std::cout<<"账号格式不规范请重新输入:";
        failtimes++;
        std::getline(std::cin,actemp);
    }
    actemp.insert(0,"(zh)");
    actemp+="(ZH)";
    Send(client_fd,actemp.c_str(),actemp.size(),0);
    x.own_account=account;
    //隐藏输入的密码
    password=getpass("请输入密码:"); 
    std::string temppas=password;
    temppas.insert(0,"(mm)");
    temppas+="(MM)";
    Send(client_fd,temppas.c_str(),temppas.size(),0);
    
    if(strncmp(a,"2",1)==0){
        std::cout<<"请输入昵称:";
        std::cin.getline(name,sizeof(name));
        std::regex alpha_only("^[A-Za-z]+$");
        while(!std::regex_match(name,alpha_only)){
            std::cout<<"名字不是纯英文字母,请重新输入:"<<std::endl;
            std::cin.getline(name,sizeof(name));
        }
        std::string tempname=name;
        tempname.insert(0,"(name)");
        tempname+="(NAME)";
        Send(client_fd,tempname.c_str(),tempname.size(),0);
    }
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
}