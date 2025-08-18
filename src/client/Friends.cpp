#include "../include/client/Friends.hpp"

Friends::Friends(std::unordered_map<int, cclientmessage>& clients, 
                   const std::string& ip, 
                   int port)
    : clientmes(clients),           
      client_ip(ip),              
      client_port(port),
      manage(clients,ip,port)           
{}

void Friends::deal_else_gn(){
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
                    std::cout<<"own mark="<<client.begin_chat_mark<<std::endl;
                    std::cout<<"group mark="<<client.if_enter_group<<std::endl;
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
        perror("deal else send");
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
            manage.groups(client_fd);
            break;
        case '8':
            new_message_show(client_fd);
            break;
        default:std::cout<<"无效选择"<<std::endl;
        break;
    }
}

void Friends::new_message_show(int client_fd){
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

void Friends::flush_recv_buffer(int sockfd){
    char buffer[4096];
    while (true) {
        ssize_t bytes = recv(sockfd, buffer, sizeof(buffer),0);
        if (bytes <= 0) {
            break; 
        }
    }
}

void Friends::deal_new_message(int client_fd,std::string message){
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

void Friends::deal_apply_mes(int client_fd,std::string message){
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

void Friends::ingore_someone(int client_fd){
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

void Friends::delete_friends(int client_fd){
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

void Friends::chat_with_friends(int client_fd,std::string request){
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
            char tempbuf[4096];
            memset(tempbuf,'\0',sizeof(tempbuf));
            std::cin.getline(tempbuf,sizeof(tempbuf));
            allbuf=tempbuf;
            if(allbuf.empty()) continue;  
            int len=allbuf.size();
            std::string headlenmes="head"+std::to_string(allbuf.size())+' ';
            allbuf.insert(0,headlenmes);

            std::string temp=allbuf;
            int n=Send(client_fd,allbuf.c_str(),allbuf.size(),0);

            if(n<0){
                send_chatting=false;
                break;
            }
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
        }
    });

    if(send_chatting==false){
        client.if_getchat_account=0;
        client.begin_chat_mark=0;
        send_chatting=true;
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

void Friends::show_friends(int client_fd){
    std::cout<<"好友列表:"<<std::endl;
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
    return;
}

void Friends::add_friends(int client_fd,std::string buf){
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
    Send(client_fd,account.c_str(),account.size(),0);
    while(!if_finshmes){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if_finshmes=false;
}

std::string Friends::check_set(std::string temp){
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