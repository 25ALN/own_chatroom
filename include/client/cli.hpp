#include "../common.hpp"
#include "ftpc.hpp"
static bool recv_chatting=true;
static bool send_chatting=true;
int Send(int fd, const char *buf, int len, int flags);
int Recv(int fd,char *buf,int len,int flag);
static bool grecv_chat=true;
static bool gsend_chat=true;

struct clientmessage{
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

class chatclient{
    public:
    chatclient(const std::string &tempip,int tempport);
    void start();
    ~chatclient();
    private:

    void connect_init();
    void caidan();
    void deal_login_mu(char *a);
    void deal_else_gn();

    void delete_friends(int client_fd);
    void show_friends(int client_fd);
    void chat_with_friends(int client_fd,std::string request);
    void add_friends(int client_fd,std::string buf);
    void ingore_someone(int client_fd);
    void deal_new_message(int client_fd,std::string message);
    void new_message_show(int client_fd);
    void groups(int client_fd);
    
    void flush_recv_buffer(int sockfd);
    std::string check_set(std::string temp);

    int check_if_newmes(int client_fd);
    //心
    void heart_check();
    void heart_work_thread();

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
    void deal_apply_mes(int client_fd,std::string message);

    long long group_number_get(long long a);

    bool if_login=false;
    bool if_finshmes=false;
    //心
    std::atomic<bool> heart_if_running{false};
    std::thread heart_beat;
    std::atomic<time_t> last_heart_time{0};

    int client_fd;
    std::unordered_map<int,clientmessage> clientmes;
    
    std::set<std::string> recv_message;  
    std::string client_ip;
    int client_port;
    std::atomic<bool>mesmark=false;
};

chatclient::chatclient(const std::string &tempip,int tempport):client_ip(tempip),client_port(tempport){

}

void chatclient::start(){
    connect_init();

    while(true){
        if(!if_login){
            caidan();
        }else{
            deal_else_gn();
        }
    }
}

void chatclient::connect_init(){
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
    heart_beat=std::thread(&chatclient::heart_work_thread,this);
    heart_beat.detach();
}

void chatclient::heart_work_thread(){
    const char heart_cmd=0x05; //表示一个字节的大小
    const int heart_time=5;
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

void chatclient::heart_check(){
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

void chatclient::caidan(){
    std::cout<<"        ---------------------       "<<std::endl;
    std::cout<<"        |      聊天室        |      "<<std::endl;
    std::cout<<"        |      1.登陆        |      "<<std::endl;
    std::cout<<"        |      2.注册        |      "<<std::endl;
    std::cout<<"        |      3.注销        |      "<<std::endl;
    std::cout<<"        |      4.退出        |      "<<std::endl;
    std::cout<<"        ---------------------       "<<std::endl;
    std::thread allrecv([&]{
        auto&client=clientmes[client_fd];
        while(true){
            static int stopmark=0;
            if(stopmark==0){
                static std::mutex recv_lock;
                std::unique_lock<std::mutex> x(recv_lock);
                char buf[1000000];
                memset(buf,'\0',sizeof(buf));
                int n=Recv(client_fd,buf,sizeof(buf),0);
                if(n==-1){
                    std::cout<<"服务器已关闭，客户端即将退出"<<std::endl;
                    close(client_fd);
                    client_fd=-1;
                    if_login=false;
                    exit(0);
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
                if((temp.find("好友请求")!=std::string::npos&&temp.find(client.own_account)!=std::string::npos)
                ||temp.find("请求加入")!=std::string::npos){
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
        if(tempc>=1&&tempc<=4){
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
    if(choose[0]!='1'&&choose[0]!='2'&&choose[0]!='3'){
        std::cout<<"无效命令，请重新输入"<<std::endl;
    }
    Send(client_fd,choose,strlen(choose),0);
    deal_login_mu(choose);
}

void chatclient::deal_login_mu(char *a){
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

void chatclient::deal_else_gn(){
    auto&client=clientmes[client_fd];
    std::cout<<"        ---------------------------       "<<std::endl;
    std::cout<<"        请选择你的需求：               "<<std::endl;
    std::cout<<"        |      1.查看好友列表       |      "<<std::endl;
    std::cout<<"        |      2.私聊某人           |      "<<std::endl;
    std::cout<<"        |      3.删除好友           |      "<<std::endl;
    std::cout<<"        |      4.添加好友           |      "<<std::endl;
    std::cout<<"        |      5.屏蔽功能           |      "<<std::endl;
    std::cout<<"        |      6.群聊功能           |      "<<std::endl;
    std::cout<<"        |      7.退出               |     "<<std::endl;
    
    std::mutex recv_mutex;
    
    std::thread message_out([&]{
        while(true){
            {
                std::unique_lock<std::mutex> lock(recv_mutex);
                if(!recv_message.empty()&&recv_message.size()!=0&&mesmark==false&&client.if_enter_group==0){
                    std::cout<<"        |      8.处理/查看"<<recv_message.size()<<"条新消息 |  "<<std::endl<<std::flush;
                    mesmark=true; 
                    break;
                }else if(!recv_message.empty()&&recv_message.size()!=0&&mesmark==false&&(client.if_enter_group==1
                ||client.begin_chat_mark==1)){
                    std::cout<<"收到新消息请稍后退出后查看"<<std::endl;
                    //mesmark=true; 
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
        }
    });
    message_out.detach();
    char choose[10];
    memset(choose,'\0',sizeof(choose));
    int tempch=0;
    //处理非法输入
    int fail_times=0;
    std::cin>>tempch;
    while(tempch<1||tempch>8||std::cin.fail()){
        if(fail_times!=0){
            std::cin>>tempch;
        }
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
        if(tempch>=1&&tempch<=8){
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
    std::string temp=std::to_string(tempch);
    strcpy(choose,temp.data());
    int n=Send(client_fd,choose,1,0);
    if(n<=0){
        perror("send");
        close(client_fd);
    }
    if(choose[0]=='7'){
        char server_repon[1024];
        memset(server_repon,'\0',1024);
        while(!if_finshmes){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if_finshmes=false;
        if_login=false;
        return;
    }
    
    char buf[1024]; 
    memset(buf,'\0',sizeof(buf));
    switch (choose[0]){
        case '1':
            show_friends(client_fd);
            break;
        case '2':
            chat_with_friends(client_fd,(std::string)buf);
            break;
        case '3':
            delete_friends(client_fd);
            break;
        case '4':
            add_friends(client_fd,"no");
            break;
        case '5':
            ingore_someone(client_fd);
            break;
        case '6':
            client.if_enter_group=1;
            groups(client_fd);
            break;
        case '8':
            new_message_show(client_fd);
            break;
        default:std::cout<<"无效选择"<<std::endl;
        break;
    }
}

void chatclient::new_message_show(int client_fd){
    if(recv_message.empty()){
        std::cout<<"当前暂无新消息"<<std::endl;
        return;
    }
    std::cout<<"请选择你的需求或查看相关信息:"<<std::endl;
    int cnt=1;
    std::unordered_map<int,std::string> temp;
    std::mutex mes_showlcok;
    std::unique_lock<std::mutex> slock(mes_showlcok);
    for(auto i:recv_message){
        if(i.find('[')==std::string::npos) continue;
        std::string checkmes=i;
        checkmes=checkmes.substr(checkmes.find('['),checkmes.find(']')+1);
        if(cnt-1>=1){
            if(temp[cnt-1].find(checkmes)!=std::string::npos){
                continue;
            }
        }
        std::cout<<std::to_string(cnt)<<"."<<checkmes<<std::endl;
        temp[cnt]=checkmes;
        cnt++;
    }
    slock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(50)); 
    
    int choose=0;
    std::cout<<"输入你想处理的消息序号:";
    int fail_times=0;
    std::cin>>choose;
    while(choose<1||choose>=cnt||std::cin.fail()){
        if(fail_times!=0){
            std::cin>>choose;
        }
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
        if(choose>=1&&choose<cnt){
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
    deal_new_message(client_fd,temp[choose]);
}

void chatclient::flush_recv_buffer(int sockfd){
    char buffer[4096];
    while (true) {
        ssize_t bytes = recv(sockfd, buffer, sizeof(buffer),0);
        if (bytes <= 0) {
            break; 
        }
    }
}

void chatclient::deal_new_message(int client_fd,std::string message){
    if(message.find("好友请求:")!=std::string::npos){
        add_friends(client_fd,message);
        flush_recv_buffer(client_fd);
        recv_message.erase(message);
    }else if(message.find("私聊消息")!=std::string::npos){
        chat_with_friends(client_fd,message);
    }else if(message.find("加入群聊")!=std::string::npos){
        deal_apply_mes(client_fd,message);
        recv_message.erase(message);
    }else{
        recv_message.erase(message);
    }
    std::string next_choose;
    mesmark=false;
    if(!recv_message.empty()&&!recv_message.count(message)){
        std::cout<<"是否继续处理消息？(1继续0退出)";
        std::getline(std::cin,next_choose);
        int failtime=1;
        while(next_choose.size()!=1||(next_choose!="1"&&next_choose!="0")){
            if(failtime==3){
                std::cout<<"输入错误次数过多请稍后重试"<<std::endl;
                return;
            }
            std::cout<<"输入格式有误，请重新输入:";
            std::getline(std::cin,next_choose);
            failtime++;
        }
        if(next_choose=="1"){
            new_message_show(client_fd);
        }else{
            return;
        }
    }else{ 
        std::cout<<"消息已全部处理完，暂时无新消息"<<std::endl;
    }
}

void chatclient::deal_apply_mes(int client_fd,std::string message){
    int pos=message.find("用户");
    std::string account=message.substr(0,pos);
    pos=message.find("聊");
    std::string group_number=message.substr(pos+3,9);
    //验证身份
    std::string mes=group_number+"identify";
    Send(client_fd,mes.c_str(),mes.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    auto&client=clientmes[client_fd];
    if(client.identify_check==1){
        std::cout<<"是否同意该用户进群(1是0否,输入1或0):";
        std::string choose;
        std::getline(std::cin,choose);
        int fail_time=0;
        while(choose.size()!=1||(choose!="0"&&choose!="1")){
            if(fail_time==3){
                std::cout<<"输入错误次数过多，请稍后重试"<<std::endl;
                return;
            }
            std::cout<<"输入格式有误，请重新输入:";
            fail_time++;
            std::getline(std::cin,choose);
        }
        if(choose=="0"){
            int pos=account.find('[');
            account.erase(pos,1);
            std::string tempmes="你的入群申请已被拒绝(";
            tempmes+=(account+")*refuse*");
            std::cout<<"tempmes="<<tempmes<<std::endl;
            Send(client_fd,tempmes.c_str(),tempmes.size(),0);
            std::cout<<"已拒绝该用户的入群申请"<<std::endl;
        }else{
            int pos=account.find('[');
            account.erase(pos,1);
            choose+="(agree enter)*"+account+"**"+group_number+"$$";
            Send(client_fd,choose.c_str(),choose.size(),0);
            std::cout<<"该用户已成功入群"<<std::endl;
        }
    }else{
        std::cout<<"你不是该群的群主或管理员无权进行处理"<<std::endl;
    }
    client.identify_check=0;
}

void chatclient::groups(int client_fd){
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

void chatclient::create_group(int client_fd,int choose){
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

void chatclient::disband_group(int client_fd,int choose){
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

void chatclient::look_enter_group(int client_fd,int choose){
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

void chatclient::apply_enter_group(int client_fd,int choose){
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

void chatclient::groups_chat(int client_fd,int choose){
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
        std::cout<<"list file列出可下载的文件"<<std::endl;
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
        // int recvBufSize = 8 * 1024 * 1024; // 8MB
        // setsockopt(group_chatfd, SOL_SOCKET, SO_RCVBUF, &recvBufSize, sizeof(recvBufSize));

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
                char gbuf[5000000];
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
                std::getline(std::cin,message);
                if(gsend_chat==false){
                    break;
                }
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
            client.if_begin_group_chat=0;
            gsend_chat=true;
            grecv_chat=true;
        }
    }else{
        return;
    }
}

void chatclient::quit_one_group(int client_fd,int choose){
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

void chatclient::look_group_members(int client_fd,int choose){
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

void chatclient::owner_charger_right(int client_fd,int choose){
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
    if(chose=="4"){
        client.identify_pd=0;
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

void chatclient::la_people_in_group(int client_fd,int choose){
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

void chatclient::move_someone_outgroup(int client_fd,std::string xz,std::string group_number){
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

void chatclient::add_group_charger(int client_fd,std::string xz,std::string group_number){
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

long long chatclient::group_number_get(long long a){
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

void chatclient::ingore_someone(int client_fd){
    std::cout<<"        请选择你的需求：               "<<std::endl;
    std::cout<<"        |      1.屏蔽某人               |      "<<std::endl;
    std::cout<<"        |      2.解除对某人的屏蔽       |      "<<std::endl;
    static int times=0;
    std::string choose;
    std::getline(std::cin,choose);
    if(choose=="1"){
        std::string account;
        std::cout<<"请输入你想屏蔽的账号:";
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
        account+="B";
        int n=Send(client_fd,account.c_str(),account.size(),0);
        if(n<0){
            perror("ingore send");
            close(client_fd);
            return;
        }
        while(!if_finshmes){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if_finshmes=false;
    }else if(choose=="2"){
        //获取屏蔽列表
        auto&client=clientmes[client_fd];
        std::string getmes=client.own_account+"ownhmd";
        Send(client_fd,getmes.c_str(),getmes.size(),0);
        while(!if_finshmes){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if_finshmes=false;
        if(client.ifhave_pb==0){
            client.ifhave_pb=-1;
            return;
        }
        std::cout<<"请输入你想要解除屏蔽的账号:";
        std::string account;
        std::getline(std::cin,account);
        int failtimes=0;
        while(account.size()!=6||!std::all_of(account.begin(),account.end(),::isdigit)){
            if(failtimes==3){
                std::cout<<"错误次数过多请稍后重试"<<std::endl;
                client.ifhave_pb=-1;
                return;
            }
            std::cout<<"账号格式不规范请重新输入:";
            failtimes++;
            std::getline(std::cin,account);
        }
        account+="K";
        Send(client_fd,account.c_str(),account.size(),0);
        while(!if_finshmes){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if_finshmes=false;
        client.ifhave_pb=-1;
    }else{
        times++;
        if(times==3){
            std::cout<<"输入错误次数过多，请稍后重试"<<std::endl;
            times=0;
            return;
        }else{
            std::cout<<"无效输入！请重新按照格式输入";
            ingore_someone(client_fd);
        }
    }
}

void chatclient::delete_friends(int client_fd){
    auto&client=clientmes[client_fd];
    char friend_name[1024];
    memset(friend_name,'\0',sizeof(friend_name));
    std::cout<<"请输入你要删除的好友的账号:";
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
    strcpy(friend_name,account.c_str());
    if(friend_name==client.own_account){
        std::cout<<"不允许删除自己!"<<std::endl;
        return;
    }
    int n=Send(client_fd,friend_name,strlen(friend_name),0);
    if(n<0){
        perror("send");
        close(client_fd);
        return;
    }
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
}

void chatclient::chat_with_friends(int client_fd,std::string request){
    auto &client=clientmes[client_fd];
    if(request.find("私聊消息")!=std::string::npos){
        std::string temp;
        std::cout<<"请选择加入私聊(1)或者等会再说(0) (输入1或0):";
        std::getline(std::cin,temp);
        while(temp.size()!=1){
            std::cout<<"非法输入!请重新按照格式进行输入:";
            std::getline(std::cin,temp);
        }
        if(temp=="1"){
            client.begin_chat_mark=1;
            int pos=request.find('(');
            int tail=request.find(')');
            std::string getaccout=request.substr(pos+1,tail-pos-1);
            std::string ensure_state="enter chat state("+getaccout+")";
            Send(client_fd,ensure_state.c_str(),ensure_state.size(),0);
            client.if_getchat_account=1;
            if(recv_message.count(request)){
                recv_message.erase(request);
            }
        }else{
            return;
        }
    }
    if(client.if_getchat_account==0){
        std::string account;
        std::cout<<"请输入你要私聊的好友账号:";
        std::getline(std::cin,account);
        int failtimes=0;
        while(account.size()!=6||!std::all_of(account.begin(),account.end(),::isdigit)){
            if(failtimes==3){
                std::cout<<"错误次数过多请稍后重试"<<std::endl;
                client.ifhave_pb=-1;
                return;
            }
            std::cout<<"账号格式不规范请重新输入:";
            failtimes++;
            std::getline(std::cin,account);
        }
        client.begin_chat_mark=1;
        if(account==client.own_account){
            std::cout<<"不允许与自己进行私聊"<<std::endl;
            return;
        }
        account+="(need to save)";
        std::string check;
        std::string temp;
        check="私聊消息";
        temp=check_set(check);
        if(temp.find("来自")!=std::string::npos&&temp.find("私聊消息")!=std::string::npos){
            std::cout<<"好友已发来私聊请求，下面进行选择"<<std::endl;
            chat_with_friends(client_fd,temp);
        }
        Send(client_fd,account.c_str(),account.size(),0);
        while(!if_finshmes){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if_finshmes=false;
        if(client.reafy_return==1){
            client.reafy_return=0;
            return;
        }
        client.if_getchat_account=1;
        std::cout<<"/exit 可退出私聊"<<std::endl;
        std::cout<<"list file可列出可下载的文件"<<std::endl;
        std::cout<<"look past可查看以往所有的历史记录"<<std::endl;
        std::cout<<"STOR + 文件 上传文件"<<std::endl;
        std::cout<<"RETR + 文件 下载文件"<<std::endl;
    }else{
        std::cout<<request<<std::endl;
    }
    std::thread send_thread([&]{
        while (send_chatting) {
            std::string allbuf;
            if(!std::getline(std::cin,allbuf)){
                std::cin.clear();
                continue;
            }
            if(allbuf.empty()) continue;  
            int len=allbuf.size();
            std::string headlenmes="head"+std::to_string(allbuf.size())+' ';
            allbuf.insert(0,headlenmes);

            std::string temp=allbuf;
            Send(client_fd,allbuf.c_str(),allbuf.size(),0);
            if(temp.substr(temp.find(' ')+1,4)=="STOR"||temp.substr(temp.find(' ')+1,4)=="RETR"){
                std::string filemes;
                if(temp.find("STOR ")!=std::string::npos){
                    int pos=temp.find("STOR ");
                    filemes=temp.substr(pos,len);
                    std::string checkpath=filemes.substr(5,filemes.size()-5);
                    if (!std::filesystem::exists(checkpath)) {
                        std::cout<<"该路径不存在"<<std::endl;
                        continue;
                    }else if (std::filesystem::is_directory(checkpath)) {
                        std::cout<<"这是一个目录,不允许上传一个目录"<<std::endl;
                        continue;
                    }
                }else{
                    int pos=temp.find("RETR ");
                    filemes=temp.substr(pos,len);
                }
                filemes.insert(0,"own");
                client.fptc.start(filemes,client_ip);
            }else if(temp.find("head5 /exit")!=std::string::npos){
                send_chatting=false;
                break;
            }
            allbuf.clear();
            temp.clear();
        }
    });

    if(send_chatting==false){
        client.if_getchat_account=0;
        client.begin_chat_mark=0;
        std::cout<<"私聊结束"<<std::endl;
    }
    if(send_thread.joinable()){
        send_thread.join();
        send_chatting=true;
        client.if_getchat_account=0;
        client.begin_chat_mark=0;
        return;
    }
}

void chatclient::show_friends(int client_fd){
    std::cout<<"好友列表:"<<std::endl;
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    return;
}

void chatclient::add_friends(int client_fd,std::string buf){
    if(buf.find("好友请求")!=std::string::npos){
        std::cout<<"请输入1(同意)或0(拒绝):"<<std::endl;
        std::string re;
        std::string choose;
        std::getline(std::cin,choose);
        while(choose.size()!=1||(choose!="1"&&choose!="0")){
            std::cout<<"输入格式有误!请重新按照格式进行输入:";
            std::getline(std::cin,choose);
        }
        if(choose=="0"){
            re="拒绝";
        }else{
            re="同意";
        }
        int n=Send(client_fd,re.c_str(),re.size(),0);
        return;
    }
    
    std::string account;
    std::cout<<"请输入你要添加好友的账号:";
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
    account+='\n';
    int n=Send(client_fd,account.c_str(),account.size(),0);
    if(n<=0){
        perror("add send");
        close(client_fd);
        return;
    }
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
}

std::string chatclient::check_set(std::string temp){
    std::string message;
    for(const std::string&str:recv_message){
        if(str.find(temp)!=std::string::npos){
            message=str;
            return message;
        }
    }
    message="no";
    return message;
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

int Recv(int fd,char *buf,int len,int flag){
    int reallen=0;
    fd_set set;
    while(reallen<len){
        int temp = recv(fd, buf + reallen, len - reallen, flag);
        if (temp > 0) {
            reallen += temp;
        } else if (temp == 0) { // 连接关闭
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 非阻塞模式下无更多数据，退出循环
            } else {
                perror("recv");
                close(fd);
                return -1;
            }
        }
    }
    return reallen;
}

int Send(int fd, const char *buf, int len, int flags)
{
    int reallen = 0;
    while (reallen < len){
        int temp = send(fd, buf + reallen, len - reallen, flags);
        if (temp < 0){
            perror("send");
            close(fd);
            return reallen;
        }
        else if (temp == 0){
            // 数据已全部发送完毕
            return reallen;
        }
        reallen += temp;
    }
    return reallen;
}