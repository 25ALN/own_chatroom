#include "../include/server/net_connect.hpp"
#include "../include/server/login_re.hpp"

net_init::net_init(redisContext* conn_, 
                   std::unordered_map<int, clientmessage>& clients, 
                   const std::string& ip, 
                   int port)
    : conn(conn_),                
      clientm(clients),           
      server_ip(ip),              
      server_port(port),          
      manage(conn_, clients),     
      gmaage(conn_, clients)     
{
    epoll_fd = -1;
    server_fd = -1;
}

void net_init::deal_new_connect(){
    struct sockaddr_in clmes;
    socklen_t len=sizeof(clmes);
    std::cout<<"有新的连接"<<std::endl;
    while(true){
        int client_fd=accept(server_fd,(struct sockaddr*)&clmes,&len);
        if(client_fd<0){
            if(errno==EAGAIN||errno==EWOULDBLOCK) break; //当前无正在等待的连接，退出循环
            else{
                perror("accept");
                close(client_fd);
                return;
            }
        }
        int flag=fcntl(client_fd,F_GETFL,0);
        fcntl(client_fd,F_SETFL,flag|O_NONBLOCK);
        char client_ip[1024];
        memset(client_ip,'\0',sizeof(client_ip));
        inet_ntop(AF_INET,&clmes.sin_addr,client_ip,sizeof(client_ip));
        clientm[client_fd].ip=client_ip;
        clientm[client_fd].port=ntohs(clmes.sin_port);
        clientm[client_fd].data_fd=-1;

        struct epoll_event ev;
        ev.data.fd=client_fd;
        ev.events=EPOLLET|EPOLLIN;
        if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&ev)<0){
            perror("epoll_ctl");
            close(client_fd);
            return;
        }
    }
}

void net_init::connect_init(){
    conn=redisConnect("127.0.0.1", 6379);
    if (conn == NULL || conn->err) {
        std::cout<<"数据库连接失败"<<std::endl;
        return;
    }else{
        std::cout<<"数据库已成功连接!"<<std::endl;
    }
    server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
        perror("socket fail");
        return;
    }
    int flag=fcntl(server_fd,F_GETFL,0);
    fcntl(server_fd,F_SETFL,flag|O_NONBLOCK);
    clientm[server_fd].ip=server_ip;
    clientm[server_fd].port=server_port;
    auto x=clientm[server_fd];
    int opt=1;
    if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int))){
        close(server_fd);
        perror("setsockopt");
    }
    struct sockaddr_in serm;
    serm.sin_family=AF_INET;
    serm.sin_port=htons(x.port);
    if(x.ip=="0.0.0.0"||x.ip.empty()){
        serm.sin_addr.s_addr=htonl(INADDR_ANY);
    }else{
        inet_pton(AF_INET,x.ip.c_str(),&serm.sin_addr);
    }
    int bind_fd=bind(server_fd,(struct sockaddr*)&serm,sizeof(serm));
    if(bind_fd<0){
        perror("bind");
        close(server_fd);
    }
    int listen_fd=listen(server_fd,5);
    if(listen_fd<0){
        perror("listen");
        close(server_fd);
    }
    clientm[server_fd].data_fd=-1;
}

void net_init::deal_client_mes(int client_fd){
    
    char buf[3000000];
    memset(buf,'\0',sizeof(buf));
    auto &client=clientm[client_fd];
    int n=Recv(client_fd,buf,sizeof(buf),0);

    if(n==1&&static_cast<unsigned char>(buf[0])==0x05){
        client.last_heart_time=time(nullptr);
        const char heart_cmd=0x06;
        Send(client_fd,&heart_cmd,1,0);
        return;
    }
    std::string temp;
    
    temp+=(std::string)buf;
    for(int i=0;i<temp.size();i++){
        if(static_cast<int>(temp[i])==5){
            temp.erase(i,1);
        }
    }    
    if(temp.find("yzm check mes")!=std::string::npos){
        find_password(client_fd,temp);
        return;
    }
    if(temp.size()>0){
        if(temp.find("head")!=std::string::npos&&temp.find("$?")==std::string::npos){
            int pos=temp.find("head");
            temp.erase(0,pos);
            std::mutex recv_lcok;
            std::unique_lock<std::mutex> buffer_lock(recv_lcok);
            while(temp.size()!=0){
                int pos1=temp.find(' ');
                int pos2=temp.find('d');
                if(pos2-pos1+temp.find("head")+4>temp.size()||pos2==-1||pos1==-1) break;
                std::string len=temp.substr(temp.find("head")+4,pos1-pos2-1);
                if(len.find('\r')!=std::string::npos||len.find('\n')!=std::string::npos||len.empty()||len.find(' ')!=std::string::npos) break;
                int meslen=std::stoi(len);
                if(meslen+4+len.size()>temp.size()) break;
                std::string tempmes=temp.substr(pos1+1,meslen);
                temp.erase(0,meslen+5+len.size());
                tempmes+="/test/";
                if(tempmes=="/exit/test/"){
                    client.if_begin_chat=0;
                    break;
                }
                recv_buffer.push_back(tempmes);
            }
            recv_lcok.unlock();
        }else if(temp.find("ghead")!=std::string::npos){
            
            std::mutex recv_lcok;
            std::unique_lock<std::mutex> buffer_lock(recv_lcok);
            while(temp.size()!=0){
                
                int pos1=temp.find(' ');
                int pos2=temp.find('d');
                if(pos1==-1||pos2==-1) break;
                if(pos2-pos1+temp.find("ghead")+5>temp.size()) break;
                std::string len=temp.substr(temp.find("ghead")+5,pos1-pos2-1);
                 if(len.find('\r')!=std::string::npos||len.find('\n')!=std::string::npos||len.empty()||len.find(' ')!=std::string::npos) break;
                int meslen=std::stoi(len);
                if(meslen+5+len.size()>temp.size()) break;
                std::string tempmes=temp.substr(pos1+1,meslen);
                temp.erase(0,meslen+6+len.size());
                if(tempmes.substr(0,6)=="/gexit"){
                    std::cout<<"清空准备信息"<<std::endl;
                    //成功进入，但消息还是客户端的群聊消息
                    client.if_begin_group_chat=0;
                    client.group_message.clear();
                    client.group_chat_num.clear();
                    break;
                }
                recv_buffer.push_back(tempmes);
            }
            recv_lcok.unlock();
        }
        else{
            recv_buffer.push_back(temp);
        }
    }
    
    if(client.state==0){
        if(temp[0]=='5'){
            find_password(client_fd,temp);
        }else{
            if(client.login_step==0){
                client.mark=buf[0];
                client.login_step=1;
            }else if(client.login_step==1){
                if(temp.find("zh")==std::string::npos) return;
                client.zh=temp.substr(temp.find("(zh)")+4,6);
                client.login_step=2;
            }else if(client.login_step==2){
                int pos1=temp.find("(mm)");
                int pos2=temp.find("(MM)");
                if(pos1==-1||pos2==-1) return;
                client.mm=temp.substr(pos1+4,pos2-pos1-4);
                if(client.mark[0]=='2'){
                    client.login_step=3;
                }else{
                    deal_login_in(client_fd);
                    client.login_step=0;
                }
            }else if(client.login_step==3){
                int pos1=temp.find("(name)");
                int pos2=temp.find("(NAME)");
                if(pos1==-1||pos2==-1) return;
                client.name=temp.substr(pos1+6,pos2-pos1-6);
                deal_login_in(client_fd);
                client.login_step=0;
            }
        }
    }else{
        std::string key="new message:messages";
        redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",key.c_str());
        if(reply->elements>0){
            std::cout<<"        ready send new message"<<std::endl;
            std::string tempresponse;
            std::string deltemp;
            for(int i=0;i<reply->elements;i++){
                std::string temp=reply->element[i]->str;
                std::cout<<"temp="<<temp<<std::endl;
                tempresponse+=temp;
                if(!tempresponse.empty()){
                    deltemp=tempresponse;
                    tempresponse+='\n';
                    break;
                }
            }
            if(!tempresponse.empty()){
                if(tempresponse.find("请求加入群聊")!=std::string::npos){
                    int n=gmaage.send_group_apply_mess(client_fd,tempresponse);
                    if(n==1){
                        std::cout<<"delmes="<<deltemp<<":end"<<std::endl;
                        redisReply *delmes=(redisReply *)redisCommand(conn,"SREM %s %s",key.c_str(),deltemp.c_str());
                        freeReplyObject(delmes);
                    }
                }else{
                    int pos1=tempresponse.find('(');
                    int pos2=tempresponse.find(')');
                    std::string mbaccount=tempresponse.substr(pos1+1,pos2-pos1-1);
                    int if_found=0;
                    int target_fd=-1;
                    for(auto&[fd,a]:clientm){
                        if(a.cur_user==mbaccount){
                            target_fd=fd;
                            if_found=1;
                            break;
                        }
                    }
                    if(if_found==1){
                        int n=Send(target_fd,tempresponse.c_str(),tempresponse.size(),0);
                        std::cout<<"had send :"<<n<<std::endl;
                        //删除新消息
                        std::string del_key="new message:messages";
                        redisReply *del=(redisReply *)redisCommand(conn,"SREM %s %s",del_key.c_str(),deltemp.c_str());
                        freeReplyObject(del);
                    }
                }
            }
        }
        std::mutex recv_lock;
        std::unique_lock<std::mutex> bufflock(recv_lock);
        for(auto&i:recv_buffer){
            if(recv_buffer.empty()) break;
            if(client.if_begin_chat==1&&!client.chat_with.empty()&&recv_buffer.front().find("list file")==std::string::npos
            &&recv_buffer.front().find("look past")==std::string::npos){
                manage.chat_with_friends(client_fd,client.chat_with,recv_buffer.front());
            }else if(client.if_begin_group_chat==1&&recv_buffer.front().find("glist file")==std::string::npos
            &&recv_buffer.front().find("glook past")==std::string::npos){
                client.group_message=recv_buffer.front();
                gmaage.groups_chat(client_fd);
            }else{
                manage.deal_friends_part(client_fd,recv_buffer.front());
            }
            recv_buffer.pop_front();
        }
        bufflock.unlock();
    }
}

void net_init::deal_login_in(int client_fd){
    auto &client = clientm[client_fd];
    login_re_zhuxiao x;
    std::string response;
    std::cout<<"zh="<<client.zh<<" mm="<<client.mm<<std::endl;
    if (client.mark[0]=='1') { // 登录
        int result = x.check_login(conn, client.zh.c_str(), client.mm.c_str());
        response=result?"登录成功":"登录失败";
        if(result){
            int ensure=-1;
            for(auto&[x,a]:clientm){
                if(a.cur_user==client.zh){
                    ensure=x;
                }
                if(ensure!=-1){
                    response="该账号已在其他地方登陆,不允许再次登陆";
                }
            }
            if(ensure==-1){
                client.state=1;
                client.cur_user=client.zh;
            }
        }
    }else if (client.mark[0]=='2') { // 注册
        if(client.name.empty()){
            std::cout<<"注册失败，昵称不能为空！"<<std::endl;
        }else{
            int result = x.check_account(conn, client.zh.c_str(), client.mm.c_str(),client.name.c_str());
            response = result ? "注册成功" : "注册失败,该账号已存在";
        }
    }else if (client.mark[0]=='3') { // 注销
        int result = x.delete_account(conn, client.zh.c_str(), client.mm.c_str());
        if(result == 1) {
            response = "注销成功";
            client.state=0;
            client.login_step=0;
        }
        else if (result == 0) response = "账号不存在";
        else response = "密码错误";
    } else {
        response = "无效命令";
    }
    std::cout<<"reponse="<<response<<std::endl;
    std::cout<<"state="<<client.state<<std::endl;
    if(response.find("登录成功")!=std::string::npos||response.find("注册成功")!=std::string::npos){
        response+=("(login success)");
    }
    int n=Send(client_fd, response.c_str(), response.size(),0);
    if(n<=0){
        perror("send");
        close(client_fd);
        return;
    }
    if(response.find("登录成功")!=std::string::npos||response.find("注册成功")!=std::string::npos){
        client.state=1;
        client.cur_user = client.zh;
    }
    // 清除临时数据
    client.mark.clear();
    client.zh.clear();
    client.mm.clear();
    client.name.clear();
}

void net_init::find_password(int client_fd,std::string mes){
    static std::string code;
    static std::string account;
    if(mes.find("yzm")==std::string::npos){
        account=mes.substr(2,6);
        std::string email=mes.substr(9,mes.size()-9);
        std::cout<<"account="<<account<<std::endl;
        std::cout<<"email="<<email<<std::endl;

        srand(time(nullptr));
        for(int i=0;i<6;i++){
            code += '0' + rand() % 10;
        }
        std::string recipient=email;
        std::string sender="3076331747@qq.com";
        std::string subject = "验证码";

        std::string cmd =
        "(echo \"From: " + sender + "\";"
        "echo \"To: " + recipient + "\";"
        "echo \"Subject: " + subject + "\";"
        "echo \"Date: $(date -R)\";"
        "echo \"MIME-Version: 1.0\";"
        "echo \"Content-Type: text/plain; charset=UTF-8\";"
        "echo \"Content-Transfer-Encoding: 8bit\";"
        "echo \"\";"
        "echo \"尊敬的用户，\";"
        "echo \"\";"
        "echo \"我们收到了您在聊天室系统中提交的账号安全验证请求。\";"
        "echo \"为了确保是您本人操作，请使用以下动态安全码完成身份验证：\";"
        "echo \"\";"
        "echo \"【" + code + "】\";"
        "echo \"\";"
        "echo \"此动态安全码仅在 5 分钟内有效，请尽快在系统中输入。\";"
        "echo \"如果您并未发起请求，请忽略本邮件。\";"
        "echo \"\";"
        "echo \"为了保障您的账号安全，请不要将此动态安全码透露给任何人。\";"
        "echo \"\") | msmtp --account=default " + recipient;


        int ret = system(cmd.c_str());
        std::string msg="验证码已发送";
        if(ret!=0){
            msg="验证码发送失败";
        }
        Send(client_fd,msg.c_str(),msg.size(),0);
    }else{
        std::string response;
        int pos=mes.find("mes");
        if(pos==-1||pos+9<mes.size()){
            response="验证码错误";
        }else{
            std::string checkcode=mes.substr(pos+3,6);
            if(checkcode!=code){
                response="验证码错误";
            }else{
                std::string password_key="user:"+account;
                redisReply *getpassword=(redisReply *)redisCommand(conn,"HGET %s password",password_key.c_str());
                std::string password=getpassword->str;
                freeReplyObject(getpassword);
                password.insert(0,"你的密码为:");
                Send(client_fd,password.c_str(),password.size(),0);
            }
        }
        if(!response.empty()&&response=="验证码错误"){
            Send(client_fd,response.c_str(),response.size(),0);
        }
        code.clear();
        account.clear();
    }
}

