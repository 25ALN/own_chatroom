#include "../include/server/Groups_manage.hpp"

GroupManager::GroupManager(redisContext* conn_, 
                           std::unordered_map<int, clientmessage>& clients)
    : conn(conn_), clientm(clients) 
{
}

void GroupManager::groups(int client_fd){
    auto &client=clientm[client_fd];
    std::string choose;
    if(!client.group_message.empty()){
        choose=client.group_message[0];
        if(client.group_message[1]=='0'){
            choose+=client.group_message[1];
            client.group_message.erase(0,3);
        }else{
            client.group_message.erase(0,2);
        }
    }
    int ch=-1;
    if(choose.empty()||choose[0]==0x06||choose[0]==0x05||choose[0]=='\n'){
        ch=-1;
    }else{
        ch=std::stoi(choose);
    }
    switch (ch){
    case 1:
        create_groups(client_fd);
        break;
    case 2:
        disband_groups(client_fd);
        break;
    case 3:
        look_enter_groups(client_fd);
        break;
    case 4:
        apply_enter_group(client_fd);
        break;
    case 5:
        groups_chat(client_fd);
        break;
    case 6:
        quit_one_group(client_fd);
        break;
    case 7:
        look_group_members(client_fd);
        break;
    case 8:
        own_charger_right(client_fd);
        break;
    case 9:
        la_people_in_group(client_fd);
        break;
    default:
        break;
    }
}

void GroupManager::create_groups(int client_fd){
    auto &client=clientm[client_fd];
    std::string group_number=client.group_message.substr(0,9);
    std::string key="group:"+group_number;
    int pos=client.group_message.find("(group)");
    std::string group_name=client.group_message.substr(9,pos-9);
    std::string response;
    std::cout<<"key:"<<key<<":end"<<std::endl;;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",key.c_str());
    if(reply->integer==1){
        response="该群号已被使用，请换个群号";
    }
    freeReplyObject(reply);
    if(response.empty()){
        std::string boss=client.cur_user;
        redisReply *cgrp=(redisReply *)redisCommand(conn,"HSET %s group_name %s group_owner %s",key.c_str(),group_name.c_str(),boss.c_str());
        if(cgrp->type==REDIS_REPLY_INTEGER){
            response="群聊创建成功";
            std::string gnme="user:"+client.cur_user;
            redisReply *getname=(redisReply *)redisCommand(conn,"HGET %s name",gnme.c_str());
            std::string tempname=(std::string)getname->str+" ";
            freeReplyObject(getname);

            std::string member_key=group_number+":member";
            std::string meb_mes=tempname+client.cur_user+"(群主)";
            std::cout<<"    member message:"<<meb_mes<<std::endl;
            redisReply *member=(redisReply *)redisCommand(conn,"SADD %s %s",member_key.c_str(),meb_mes.c_str());
            freeReplyObject(member);

            std::string own_joing=client.cur_user+":joined_groups";
            std::string gmesa=group_number+"   "+group_name+" "+client.cur_user+"(群主)";
            redisReply *ownj=(redisReply *)redisCommand(conn,"SADD %s %s",own_joing.c_str(),gmesa.c_str());
            freeReplyObject(ownj);
        }else{
            response="群聊创建失败";
        }
        freeReplyObject(cgrp);
    }
    std::cout<<"create response:"<<response<<std::endl;
    int n=Send(client_fd,response.c_str(),response.size(),0);
    if(n<0){
        perror("create send");
    }
    client.group_message.clear();
}

void GroupManager::disband_groups(int client_fd){
    std::cout<<"enter disband"<<std::endl;
    auto&client=clientm[client_fd];
    std::string group_num=client.group_message.substr(0,9);
    std::string key="group:"+group_num;
    std::string response;
    std::cout<<"key="<<key<<":end"<<std::endl;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",key.c_str());
    if(reply->integer!=1){
        response="该群号不存在";
    }
    freeReplyObject(reply);
    if(response.empty()){
        std::string own_key="group:"+group_num;
        redisReply *qz=(redisReply *)redisCommand(conn,"HGET %s group_owner",own_key.c_str());
        std::string temp=qz->str;
        std::cout<<"temp="<<temp<<std::endl;
        if(temp.find(client.cur_user)==std::string::npos){
            response="你不是该群群主，没有解散该群的权限";
        }
        freeReplyObject(qz);
    }
    if(response.empty()){
        redisReply *disband=(redisReply *)redisCommand(conn,"DEL %s",key.c_str());
        std::string member_key=group_num+":member";
        //通知群已被解散了
        int online=0;
        redisReply *getmember=(redisReply *)redisCommand(conn,"SMEMBERS %s",member_key.c_str());
        std::string mes=group_num+"群已被解散";
        mes+=0x07;
        for(int i=0;i<getmember->elements;i++){
            std::string temp=getmember->element[i]->str;
            std::string account=temp.substr(temp.find(' ')+1,6);
            std::string joinkey=account+":joined_groups";
            redisReply *deljoin=(redisReply *)redisCommand(conn,"SMEMBERS %s",joinkey.c_str());
            for(int i=0;i<deljoin->elements;i++){
                std::string delj=deljoin->element[i]->str;
                if(delj.find(group_num)!=std::string::npos){
                    redisReply *dels=(redisReply *)redisCommand(conn,"SREM %s %s",joinkey.c_str(),delj.c_str());
                    freeReplyObject(dels);
                    break;
                }
            }
            freeReplyObject(deljoin);
            for(auto&[fd,a]:clientm){
                if(account==client.cur_user) break;
                if(a.cur_user==account){
                    if(a.if_begin_group_chat==1){
                        mes+="请输入/gexit退出群聊";
                        Send(fd,mes.c_str(),mes.size(),0);
                    }else{
                        Send(fd,mes.c_str(),mes.size(),0);
                    }
                    online=1;
                    break;
                }
            }
            if(online==0){
                std::string newmes="new message:message";
                mes+='('+account+')';
                redisReply *savemes=(redisReply *)redisCommand(conn,"SADD %s %s",newmes.c_str(),mes.c_str());
                freeReplyObject(savemes);
            }
        }
        freeReplyObject(getmember);
        redisReply *delmembers=(redisReply *)redisCommand(conn,"DEL %s",member_key.c_str());
        if(disband->integer==1){
            response="已成功解散该群聊";
            std::string ghistory_key=group_num+":ghistory";
            redisReply *delgh=(redisReply *)redisCommand(conn,"DEL %s",ghistory_key.c_str());
            freeReplyObject(delgh);
        
        }else{
            response="未知错误";
        }
        freeReplyObject(disband);
        freeReplyObject(delmembers);
    }
    std::cout<<"disband response:"<<response<<std::endl;
    int n=Send(client_fd,response.c_str(),response.size(),0);
    if(n<0){
        perror("disband send");
    }
    client.group_message.clear();
}

void GroupManager::look_enter_groups(int client_fd){
    std::cout<<"look enter g"<<std::endl;
    auto &client=clientm[client_fd];
    std::string group_key=client.cur_user+":joined_groups";
    redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",group_key.c_str());
    std::string response;
    for(int i=0;i<reply->elements;i++){
        std::cout<<"num="<<reply->elements<<std::endl;
        std::string temp=reply->element[i]->str;
        std::string del_mes=temp;
        if(del_mes.find('(')==std::string::npos){
            continue;
        }
        temp=temp.substr(0,9);
        //确保该群没有被解散
        std::string check_key="group:"+temp;
        redisReply *check=(redisReply *)redisCommand(conn,"EXISTS %s",check_key.c_str());
        if(check->integer!=1){
            redisReply *delg=(redisReply *)redisCommand(conn,"SREM %s %s",group_key.c_str(),del_mes.c_str());
            freeReplyObject(delg);
        }else{
            response+=reply->element[i]->str;
            response+='\n';
        }
        freeReplyObject(check);
        if(strlen(reply->element[i]->str)<2){
            continue;
        }
    }
    freeReplyObject(reply);
    if(response.empty()){
        response="你还未加入任何群";
    }
    std::cout<<"look enter response:"<<response<<std::endl;
    response+="(look enter group)";
    Send(client_fd,response.c_str(),response.size(),0);
    client.group_message.clear();
    return;
}

void GroupManager::apply_enter_group(int client_fd){
    std::cout<<"enter apply group"<<std::endl;
    auto &client=clientm[client_fd];
    std::string group_number=client.group_message.substr(0,9);
    std::string key="group:"+group_number;
    std::string response;
    std::cout<<"apply group number:"<<group_number<<std::endl;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",key.c_str());
    if(reply->integer!=1){
        response="该群聊不存在";
    }
    freeReplyObject(reply);
    if(response.empty()){
        std::string join_key=client.cur_user+":joined_groups";
        redisReply *r=(redisReply *)redisCommand(conn,"SMEMBERS %s",join_key.c_str());
        int if_find=0;
        for(int i=0;i<r->elements;i++){
            std::string temp=r->element[i]->str;
            std::cout<<"aply temp:"<<temp<<std::endl;
            if(temp.find('(')==std::string::npos) continue;
            if(temp.find(group_number)!=std::string::npos){
                if_find=1;
                break;
            }
        }
        if(if_find==1){
            response="你已经在该群聊中了";
        }
        freeReplyObject(r);
    }
    if(response.empty()){
        std::string to_charge=client.cur_user+"用户请求加入群聊"+group_number;
        std::string temp="new message";
        std::string apply_save=temp+":messages";
        std::cout<<"apply_save:"<<apply_save<<std::endl;
        to_charge.insert(0,"[");
        to_charge+="]("+client.cur_user+')';
        redisReply *save_message=(redisReply *)redisCommand(conn,"SADD %s %s",apply_save.c_str(),to_charge.c_str());
        freeReplyObject(save_message);
        response="已成功发送加群申请,等待管理员审核";
        std::cout<<"message="<<to_charge<<std::endl;
        int mark=send_group_apply_mess(client_fd,to_charge);
        if(mark==1){
            redisReply *delmes=(redisReply *)redisCommand(conn,"SREM %s %s",apply_save.c_str(),to_charge.c_str());
            std::cout<<"readt to del"<<std::endl;
            freeReplyObject(delmes);
        }
    }
    std::cout<<"apply enter response:"<<response<<std::endl;
    int n=Send(client_fd,response.c_str(),response.size(),0);
    if(n<0){
        perror("apply send");
    }
    client.group_message.clear();
}

void GroupManager::groups_chat(int client_fd){
    auto&client=clientm[client_fd];
    std::string message=client.group_message;
    std::string response;
    std::string group_number;
    if(client.if_begin_group_chat==0){
        group_number=message.substr(0,9);
        std::string g_key="group:"+group_number;
        redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",g_key.c_str());
        if(reply->integer!=1){
            response="该群聊不存在";
        }
        freeReplyObject(reply);
        if(response.empty()){
            int find=0;
            std::string join_key=client.cur_user+":joined_groups";
            redisReply *joined=(redisReply *)redisCommand(conn,"SMEMBERS %s",join_key.c_str());
            for(int i=0;i<joined->elements;i++){
                std::string temp=joined->element[i]->str;
                if(temp.find('(')==std::string::npos) continue;
                if(temp.find(group_number)!=std::string::npos){
                    find=1;
                    break;
                }
            }
            if(find==0){
                response="你还未加入该群聊";
            }
            freeReplyObject(joined);
        }
        if(response.empty()){
            response="可以在该群中发送消息了";
            client.group_chat_num=group_number;
            client.if_begin_group_chat=1;
        }
        if(response=="可以在该群中发送消息了"){
            std::string historymes="前50条历史消息记录:\n";
            std::string h_key=group_number+":ghistory";
            redisReply *gethistory=(redisReply *)redisCommand(conn,"LRANGE %s -50 -1",h_key.c_str());
            if(gethistory->type==REDIS_REPLY_ARRAY){
                for(int i=0;i<gethistory->elements;i++){
                    historymes+=gethistory->element[i]->str;
                    historymes+='\n';
                }
            }
            freeReplyObject(gethistory);
            response.insert(0,historymes);
            response+="(group chat begin)";
        }
        std::cout<<"response="<<response<<std::endl;
        int n=Send(client_fd,response.c_str(),response.size(),0);
        if(response.find("可以在")==std::string::npos){
            client.group_message.clear();
            return;
        }
        if(n<0){
            perror("gchat send");
        }
    }
    if(response.empty()){
        int pos=message.find("*g*(group)");
        if(pos!=-1){
            message.erase(pos,10);
        }
    }
    static int groups_chatfd;
    static struct sockaddr_in dbaddr;
    if(client.if_begin_group_chat==1&&response.find("可以在")!=std::string::npos){
        groups_chatfd=socket(AF_INET,SOCK_DGRAM,0);
        if(groups_chatfd<0){
            perror("udp socket");
            close(groups_chatfd);
            return;
        }
        //决定数据包最多可以通过多少个路由器,防止数据包一直在网络中无限制的传播
        unsigned char ttl=1;
        if(setsockopt(groups_chatfd,IPPROTO_IP,IP_MULTICAST_TTL,&ttl,sizeof(ttl))<0){
            perror("udp setsockopt");
            close(groups_chatfd);
            return;
        }
        int size = 2048*2048;
        setsockopt(groups_chatfd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));

        std::string gb_addr="239.";
        long long getaddr=std::stoll(group_number);
        gb_addr+=(std::to_string(getaddr%255)+'.');
        getaddr/=255;
        gb_addr+=(std::to_string(getaddr%255)+'.');
        getaddr/=255;
        gb_addr+=(std::to_string(getaddr%255));
        std::cout<<"ip="<<gb_addr.c_str()<<std::endl;
        memset(&dbaddr,0,sizeof(dbaddr));
        dbaddr.sin_family=AF_INET;
        dbaddr.sin_port=htons(8888);
        dbaddr.sin_addr.s_addr=inet_addr(gb_addr.c_str());
        client.group_message.clear();
        return;
    }
    if(!message.empty()){

        int pos3=message.find("$?");
        int pos4=message.find("^!");

        std::string tempmes=message.substr(0,pos3);
        std::string account=message.substr(pos3+2,6);
        message.erase(0,pos4+2);
        
        std::string name;
        std::string get_namekey="user:"+client.cur_user;
        redisReply *getn=(redisReply *)redisCommand(conn,"HGET %s name",get_namekey.c_str());
        name=getn->str;
        freeReplyObject(getn);
        name+="]:";
        name.insert(0,"[");
        tempmes.insert(0,name);
        std::string save_message=tempmes;
        tempmes+="$?"+account+"^!";
        tempmes+=0x02;
        std::string g_key="group:"+client.group_chat_num;
        std::mutex savemeslock;
        {
            std::unique_lock<std::mutex> saveLock(savemeslock);
            redisReply *greply=(redisReply *)redisCommand(conn,"EXISTS %s",g_key.c_str());
            if(greply&&greply->type==REDIS_REPLY_INTEGER&&greply->integer!=1){
                tempmes="该群已被解散，不允许继续发送消息";
                client.if_begin_group_chat=0;
                client.group_message.clear();
                client.group_chat_num.clear();
                message.clear();
                
                recv_buffer.clear();
                tempmes+=0x02;
                groups_chatfd=-1;
                memset(&dbaddr,0,sizeof(dbaddr));
            }
            freeReplyObject(greply);
        }
        int n=sendto(groups_chatfd,tempmes.c_str(),tempmes.size(),0,(struct sockaddr*)&dbaddr,sizeof(dbaddr));
        if(tempmes.find("解散")==std::string::npos){
            std::string mes=client.group_chat_num+"群中有新消息";
            mes+=0x07;
            std::string memkey=client.group_chat_num+":member";
            redisReply *getmember=(redisReply *)redisCommand(conn,"SMEMBERS %s",memkey.c_str());
            int online=0;
            for(int i=0;i<getmember->elements;i++){
                std::string temp=getmember->element[i]->str;
                std::string account2=temp.substr(temp.find(' ')+1,6);
                if(account2==client.cur_user) continue;
                for(auto&[fd,a]:clientm){
                    if(a.cur_user==account2&&a.if_begin_group_chat==0){
                        Send(fd,mes.c_str(),mes.size(),0);
                        online=1;
                        break;
                    }
                }
                if(online==0){
                    std::string newmes="new message:message";
                    mes+='('+account2+')';
                    redisReply *savemes=(redisReply *)redisCommand(conn,"SADD %s %s",newmes.c_str(),mes.c_str());
                    freeReplyObject(savemes);
                }
            }
            freeReplyObject(getmember);
        }
        if(!save_message.empty()){
            if(save_message.find("]:")+2!=save_message.size()){
                if(save_message.find("*g*(group)")!=std::string::npos&&save_message[save_message.size()-1]==')'){
                    save_message.erase(save_message.find("*g*(group)"),10);
                }
                std::string grouphistory=client.group_chat_num+":ghistory";
                redisReply *reply=(redisReply *)redisCommand(conn,"RPUSH %s %s",grouphistory.c_str(),save_message.c_str());
                freeReplyObject(reply);
            }
        }
        std::cout<<"had send n:"<<n<<std::endl;
        if(n<0){
            if(errno!=EAGAIN&&errno!=EWOULDBLOCK){
                perror("sendto");
                close(groups_chatfd);
            }else{
                client.group_message.clear();
                return;
            }
        }
        client.group_message.clear();
    }
}

void GroupManager::check_identify(int client_fd,std::string mes){
    auto&client=clientm[client_fd];
    std::string group_number=mes.substr(0,9);
    std::string key=client.cur_user+":joined_groups";
    std::cout<<"key="<<key<<std::endl;
    redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",key.c_str());
    std::string response="no find";
    for(int i=0;i<reply->elements;i++){
        std::string temp=reply->element[i]->str;
        std::cout<<"temp mess="<<temp<<std::endl;
        if(temp.find('(')==std::string::npos) continue;
        if(temp.find(group_number)!=std::string::npos){
            std::cout<<"idnetify="<<std::endl;
            if(temp.find("群主")!=std::string::npos||temp.find("管理员")!=std::string::npos){
                response="find";
                break;
            }
        }
    }
    freeReplyObject(reply);
    std::cout<<"if find:"<<response<<std::endl;
    int n=Send(client_fd,response.c_str(),response.size(),0);
    std::cout<<"find n="<<n<<std::endl;
}

void GroupManager::add_user_enterg(int client_fd,std::string mes){
    auto&client=clientm[client_fd];
    int pos=mes.find(")*");
    int pos2=mes.find("**");
    int p3=mes.find("$$");
    std::string account=mes.substr(pos+2,pos2-pos-2);
    std::string group_number=mes.substr(pos2+2,p3-pos2-2);
    std::cout<<"account="<<account<<" size="<<account.size()<<std::endl;
    std::string clean;
    for (char c:account) {
        if (std::isprint(c)) {
            clean+=c;
        }
    }
    account=clean;
    std::cout<<"group_number="<<group_number<<std::endl;
    std::string group_key=group_number+":member";
    std::string name_key="user:"+account;
    std::cout<<"name_key="<<name_key<<":end"<<std::endl;
    redisReply *getname=(redisReply *)redisCommand(conn,"HGET %s name",name_key.c_str());
    std::string uesr_name;
    
    if(getname->str!=nullptr&&strlen(getname->str)>0){
        uesr_name=getname->str;
    }
    std::cout<<"user_name="<<uesr_name<<":end"<<std::endl;
    if(uesr_name.empty()){
        std::cout<<"enter error fuzhi"<<std::endl;
        return;
    }
    freeReplyObject(getname);
    std::string meb_mes=uesr_name+" "+account+"(成员)";
    //变成一个成员
    redisReply *reply=(redisReply *)redisCommand(conn,"SADD %s %s",group_key.c_str(),meb_mes.c_str());
    freeReplyObject(reply);

    std::string joinkey=account+":joined_groups";
    std::string key="group:"+group_number;
    redisReply *getgname=(redisReply *)redisCommand(conn,"HGET %s group_name",key.c_str());
    std::string group_name=getgname->str;
    std::cout<<"group_name="<<group_name<<std::endl;

    freeReplyObject(getgname);
    redisReply *getown=(redisReply *)redisCommand(conn,"HGET %s group_owner",key.c_str());
    std::string owner=getown->str;
    freeReplyObject(getown);
    std::string joinmes=group_number+"   "+group_name+" "+owner+("(群主)");
    redisReply *joinown=(redisReply *)redisCommand(conn,"SADD %s %s",joinkey.c_str(),joinmes.c_str());
    freeReplyObject(joinown);

    int if_found=0;
    std::cout<<"size="<<account.size()<<std::endl;
    for(int j=0;j<account.size();j++){
        if(account[j]==0x05||account[j]==0x06){
            account.erase(j,1);
        }
    }
    std::cout<<"second size="<<account.size()<<std::endl;
    for(auto&[fd,a]:clientm){
        if(a.cur_user==account){
            std::string buf="[你已成功加入"+group_number+"群聊]("+account+')';
            Send(fd,buf.c_str(),buf.size(),0);
            std::cout<<"li ke send message"<<std::endl;
            if_found=1;
            break;
        }
    }
    if(if_found==0){
        std::string mes_key="new message:messages";
        std::string buf="[你已成功加入"+group_number+"群聊]("+account+')';
        redisReply *sueg=(redisReply *)redisCommand(conn,"SADD %s %s",mes_key.c_str(),buf.c_str());
        freeReplyObject(sueg);
    }
}

void GroupManager::quit_one_group(int client_fd){
    auto &client=clientm[client_fd];
    std::string group_number=client.group_message.substr(0,9);
    std::string key="group:"+group_number;
    std::string response;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",key.c_str());
    if(reply->integer!=1){
        response="该群聊不存在";
    }
    if(response.empty()){
        std::string own_key=client.cur_user+":joined_groups";
        redisReply *r=(redisReply *)redisCommand(conn,"SMEMBERS %s",own_key.c_str());
        int if_find=0;
        for(int i=0;i<r->elements;i++){
            std::string temp=r->element[i]->str;
            if(temp.find('(')==std::string::npos) continue;
            if(temp.find(group_number)!=std::string::npos){
                if_find=1;
                break;
            }
        }
        if(if_find==0){
            response="你还不是该群的成员，无法退群";
        }
        freeReplyObject(r);
    }
    if(response.empty()){
        //判断是否是群主退群
        std::string owner="group:"+group_number;
        redisReply *own=(redisReply *)redisCommand(conn,"HGET %s group_owner",owner.c_str());
        if(client.cur_user.find(own->str)!=std::string::npos){
            std::cout<<"own->str:"<<own->str<<std::endl;
            redisReply *delg=(redisReply *)redisCommand(conn,"DEL %s",owner.c_str());
            freeReplyObject(delg);

            std::string history_key=group_number+":ghistory";
            redisReply *delhis=(redisReply *)redisCommand(conn,"DEL %s",history_key.c_str());
            freeReplyObject(delhis);

            std::string mem_key=group_number+":member";
            redisReply *delmeb=(redisReply *)redisCommand(conn,"DEL %s",mem_key.c_str());
            freeReplyObject(delmeb);

            //通知群已被解散了
            int online=0;
            redisReply *getmember=(redisReply *)redisCommand(conn,"SMEMBERS %s",mem_key.c_str());
            std::string mes=group_number+"群已被解散";
            for(int i=0;i<getmember->elements;i++){
                std::string temp=getmember->element[i]->str;
                std::string account=temp.substr(temp.find(' ')+1,6);
                std::string joinkey=account+":joined_groups";
                redisReply *deljoin=(redisReply *)redisCommand(conn,"SMEMBERS %s",joinkey.c_str());
                for(int i=0;i<deljoin->elements;i++){
                    std::string delj=deljoin->element[i]->str;
                    if(delj.find(group_number)!=std::string::npos){
                        redisReply *dels=(redisReply *)redisCommand(conn,"SREM %s %s",joinkey.c_str(),delj.c_str());
                        freeReplyObject(dels);
                        break;
                    }
                }
                freeReplyObject(deljoin);
                for(auto&[fd,a]:clientm){
                    if(a.cur_user==account){
                        Send(fd,mes.c_str(),mes.size(),0);
                        online=1;
                        break;
                    }
                }
                if(online==0){
                    std::string newmes="new message:message";
                    mes+='('+account+')';
                    redisReply *savemes=(redisReply *)redisCommand(conn,"SADD %s %s",newmes.c_str(),mes.c_str());
                    freeReplyObject(savemes);
                }
            }

            response="已成功退出该群(由于你是群主，该群已解散)";
        }else{
            std::string member_key=group_number+":member";
            redisReply *meb=(redisReply *)redisCommand(conn,"SMEMBERS %s",member_key.c_str());
            for(int i=0;i<meb->elements;i++){
                std::string temp=meb->element[i]->str;
                if(temp.find(client.cur_user)!=std::string::npos){
                    redisReply *delm=(redisReply *)redisCommand(conn,"SREM %s %s",member_key.c_str(),temp.c_str());
                    freeReplyObject(delm);
                }
            }
            freeReplyObject(meb);

            //删除指定的加入的群
            std::string g_key="group:"+group_number;
            redisReply *getname=(redisReply *)redisCommand(conn,"HGET %s group_name",g_key.c_str());
            std::string group_name=getname->str;
            freeReplyObject(getname);

            redisReply *getowner=(redisReply *)redisCommand(conn,"HGET %s group_owner",g_key.c_str());
            std::string owner=getowner->str;
            freeReplyObject(getowner);

            std::string joined_key=client.cur_user+":joined_groups";
            std::string joinmes=group_number+"   "+group_name+" "+owner+"(群主)";
            std::cout<<"joinmes="<<joinmes<<std::endl;
            redisReply *joi=(redisReply *)redisCommand(conn,"SREM %s %s",joined_key.c_str(),joinmes.c_str());
            freeReplyObject(joi);
            response="已成功退出该群";
        }
        freeReplyObject(own);
    }
    int n=Send(client_fd,response.c_str(),response.size(),0);
    if(n<0){
        perror("quit send");
    }
    client.group_message.clear();
}

void GroupManager::look_group_members(int client_fd){
    auto &client=clientm[client_fd];
    std::string group_number=client.group_message.substr(0,9);
    std::string key="group:"+group_number;
    std::string response;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",key.c_str());
    if(reply->integer!=1){
        response="该群号不存在";
    }
    freeReplyObject(reply);
    //检查是否是该群的成员
    if(response.empty()){
        std::string own_key=client.cur_user+":joined_groups";
        redisReply *r=(redisReply *)redisCommand(conn,"SMEMBERS %s",own_key.c_str());
        int if_find=0;
        for(int i=0;i<r->elements;i++){
            std::string temp=r->element[i]->str;
            if(temp.find('(')==std::string::npos) continue;
            if(temp.find(group_number)!=std::string::npos){
                if_find=1;
                break;
            }
        }
        if(if_find==0){
            response="你还不是该群的成员，无法查看群成员";
        }
        freeReplyObject(r);
    }
    if(response.empty()){
        std::string look_allkey=group_number+":member";
        redisReply *loall=(redisReply *)redisCommand(conn,"SMEMBERS %s",look_allkey.c_str());
        for(int i=0;i<loall->elements;i++){
            if(strlen(loall->element[i]->str)>2){
                response+=loall->element[i]->str;
                response+='\n';
            }
        }
        freeReplyObject(loall);
    }
    std::cout<<"group members response:"<<response<<std::endl;
    int n=Send(client_fd,response.c_str(),response.size(),0);
    if(n<0){
        perror("look send");
    }
    client.group_message.clear();
}

void GroupManager::own_charger_right(int client_fd){
    std::cout<<"enter own_charge"<<std::endl;
    auto &client=clientm[client_fd];
    std::string response;
    std::cout<<"group mes:"<<client.group_message<<std::endl;
    if(client.group_message=="(group)"&&client.if_check_identity==0&&client.group_message.find("(group owner)")==std::string::npos){
        std::string key=client.cur_user+":joined_groups";
        redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",key.c_str());
        for(int i=0;i<reply->elements;i++){
            std::string temp=reply->element[i]->str;
            std::cout<<"identify check temp:"<<temp<<std::endl;
            if(temp.find('(')==std::string::npos) continue;
            std::string group_number=temp.substr(0,9);
            //验证用户在这个群中是不是管理员或群主
            std::string member_key=group_number+":member";
            redisReply *checkc=(redisReply *)redisCommand(conn,"SMEMBERS %s",member_key.c_str());
            for(int j=0;j<checkc->elements;j++){
                std::string mes=checkc->element[j]->str;
                if(mes.find(client.cur_user)!=std::string::npos&&mes.find("管理员")!=std::string::npos){
                    response="管理员";
                    response+=("(identify)");
                    client.if_charger=1;
                    break;
                }else if(mes.find(client.cur_user)!=std::string::npos&&mes.find("群主")!=std::string::npos){
                    response="群主";
                    response+=("(identify)");
                    client.if_charger=1;
                    break;
                }
            }
            freeReplyObject(checkc);
        }
        freeReplyObject(reply);
        if(response.empty()){
            response="成员";
            response+=("(identify)");
        }
        std::cout<<"will response:"<<response<<std::endl;
        Send(client_fd,response.c_str(),response.size(),0);
        client.if_check_identity=1;
        client.group_message.clear();
    }else if(client.group_message.find("(group owner)")!=std::string::npos&&client.group_message.size()>10){
        std::string choose;
        choose=client.group_message[0];
        client.group_message.erase(0,2);
        int pos1=client.group_message.find("*");
        int pos2=client.group_message.find("(group)");
        std::string account=client.group_message.substr(0,pos1);
        std::string group_number=client.group_message.substr(pos1+1,pos2-pos1-1);
        int mark=check_acount_ifexists(client_fd,account,group_number);
        std::cout<<"       many mark="<<mark<<std::endl;
        
        if(mark==4||mark==5){
            if(choose=="1"){
                std::string checkagain="own"+client.cur_user;
                int check=check_acount_ifexists(client_fd,checkagain,group_number);
                if(check!=5&&check!=6){
                    std::cout<<"check="<<check<<std::endl;
                    std::string msg="你已不再是管理员，操作无效";
                    Send(client_fd,msg.c_str(),msg.size(),0);
                    client.if_check_identity=0;
                    client.group_message.clear();
                    return;
                }
                

                std::string member_key=group_number+":member";
                redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",member_key.c_str());
                for(int i=0;i<reply->elements;i++){
                    std::string temp=reply->element[i]->str;
                    if(temp.find(account)!=std::string::npos){
                        redisReply *del=(redisReply *)redisCommand(conn,"SREM %s %s",member_key.c_str(),temp.c_str());
                        freeReplyObject(del);
                        break;
                    }
                }
                freeReplyObject(reply);
                //删除对方已加入的群聊
                std::string joinkey=account+":joined_groups";
                redisReply *jo=(redisReply *)redisCommand(conn,"SMEMBERS %s",joinkey.c_str());
                for(int i=0;i<jo->elements;i++){
                    std::string temp=jo->element[i]->str;
                    if(temp.find(group_number)!=std::string::npos){
                        redisReply *del=(redisReply *)redisCommand(conn,"SREM %s %s",joinkey.c_str(),temp.c_str());
                        freeReplyObject(del);
                        break;
                    }
                }
                freeReplyObject(jo);
                response="已成功清除该群成员";
                int if_found=0;
                for(auto&[fd,a]:clientm){
                    if(a.cur_user==account){
                        std::string buf="[你已被踢出"+group_number+"群聊]("+account+')';
                        buf+=0x07;
                        Send(fd,buf.c_str(),buf.size(),0);
                        if_found=1;
                        break;
                    }
                }
                if(if_found==0){
                    std::string  key="new message:messages";
                    std::string mes="[你已被踢出"+group_number+"群聊]("+account+')';
                    mes+=0x07;
                    redisReply *savemes=(redisReply *)redisCommand(conn,"SADD %s %s",key.c_str(),mes.c_str());
                    freeReplyObject(savemes);
                }
            }else if(choose=="2"){
                if(mark==5){
                    response="该用户已经是管理员了";
                }else{
                    std::string member_key=group_number+":member";
                    redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",member_key.c_str());
                    std::string use_mes;
                    for(int i=0;i<reply->elements;i++){
                        use_mes=reply->element[i]->str;
                        if(use_mes.find(account)!=std::string::npos){
                            redisReply *del=(redisReply *)redisCommand(conn,"SREM %s %s",member_key.c_str(),use_mes.c_str());
                            freeReplyObject(del);
                            break;
                        }
                    }
                    freeReplyObject(reply);
                    int pos1=use_mes.find("(成员)");
                    int pos2=use_mes.find(')');
                    use_mes.erase(pos1,pos2-pos1+1);
                    use_mes+="(管理员)";
                    redisReply *addch=(redisReply *)redisCommand(conn,"SADD %s %s",member_key.c_str(),use_mes.c_str());
                    freeReplyObject(addch);
                    response="已成功将该用户添加为管理员";

                    int if_found=0;
                    for(auto&[fd,a]:clientm){
                        if(a.cur_user==account){
                            std::string buf="[你已被设置为"+group_number+"群聊的管理员]("+account+')';
                            Send(fd,buf.c_str(),buf.size(),0);
                            if_found=1;
                            break;
                        }
                    }
                    if(if_found==0){
                        std::string  key="new message:messages";
                        std::string mes="[你已被设置为"+group_number+"群聊的管理员]("+account+')';
                        redisReply *savemes=(redisReply *)redisCommand(conn,"SADD %s %s",key.c_str(),mes.c_str());
                        freeReplyObject(savemes);
                    }
                }
            }else if(choose=="3"){
                if(mark==4){
                    response="该用户已经是成员了";
                }else{
                    std::string member_key=group_number+":member";
                    redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",member_key.c_str());
                    std::string user_mes;
                    for(int i=0;i<reply->elements;i++){
                        user_mes=reply->element[i]->str;
                        if(user_mes.find(account)!=std::string::npos){
                            redisReply *del=(redisReply *)redisCommand(conn,"SREM %s %s",member_key.c_str(),user_mes.c_str());
                            freeReplyObject(del);
                            break;
                        }
                    }
                    freeReplyObject(reply);
                    int pos1=user_mes.find("(管理员)");
                    int pos2=user_mes.find(')');
                    user_mes.erase(pos1,pos2-pos1+1);
                    user_mes+="(成员)";
                    redisReply *addme=(redisReply *)redisCommand(conn,"SADD %s %s",member_key.c_str(),user_mes.c_str());
                    freeReplyObject(addme);
                    response="已成功将该用户的管理员身份移除";

                    int if_found=0;
                    for(auto&[fd,a]:clientm){
                        if(a.cur_user==account){
                            std::string buf="[你在"+group_number+"群聊的管理员身份已被移除]("+account+')';
                            Send(fd,buf.c_str(),buf.size(),0);
                            if_found=1;
                            break;
                        }
                    }
                    if(if_found==0){
                        std::string  key="new message:messages";
                        std::string mes="[你在"+group_number+"群聊的管理员身份已被移除]("+account+')';
                        redisReply *savemes=(redisReply *)redisCommand(conn,"SADD %s %s",key.c_str(),mes.c_str());
                        freeReplyObject(savemes);
                    }
                }
            }
        }else{
            if(mark==-1){
                response="不允许对自己进行操作";
            }else if(mark==1){
                response="该账号不存在";
            }else if(mark==2){
                response="该群聊账号不存在";
            }else if(mark==3){
                response="该用户不属于这个群聊";
            }else if(mark==6){
                response="该用户是群主，你无权对其进行操作";
            }
        }
        Send(client_fd,response.c_str(),response.size(),0);
    }
    client.if_check_identity=0;
    client.group_message.clear();
}

void GroupManager:: la_people_in_group(int client_fd){
    auto&client=clientm[client_fd];
    std::cout<<"group la mes="<<client.group_message<<" size="<<client.group_message.size()<<std::endl;
    if(client.group_message.empty()){ 
        return;
    }
    std::string account=client.group_message.substr(0,6);
    std::string group_number=client.group_message.substr(8,9);
    std::string response;
    std::string user_key="user:"+account;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",user_key.c_str());
    if(reply->integer!=1){
        response="该账号不存在";
    }
    freeReplyObject(reply);

    if(response.empty()){
        std::string g_key="group:"+group_number;
        redisReply *gcheck=(redisReply *)redisCommand(conn,"EXISTS %s",g_key.c_str());
        if(gcheck->integer!=1){
            response="该群号不存在";
        }
        freeReplyObject(gcheck);
    }

    if(response.empty()){
        std::string mem_key=group_number+":member";
        redisReply *memcheck=(redisReply *)redisCommand(conn,"SMEMBERS %s",mem_key.c_str());
        int mark=0;
        for(int i=0;i<memcheck->elements;i++){
            std::string temp=memcheck->element[i]->str;
            if(temp.find(account)!=std::string::npos){
                mark=2;
                break;
            }else if(temp.find(client.cur_user)!=std::string::npos){
                mark=1;
            }
        }
        freeReplyObject(memcheck);
        if(mark==0){
            response="你不是该群的成员，无权拉人进群";
        }else if(mark==2){
            response="该用户已在该群中了，不要重复拉取!";
        }
    }
    if(response.empty()){
        response="已成功拉对方入群";
    }
    std::cout<<"response="<<response<<std::endl;
    Send(client_fd,response.c_str(),response.size(),0);
    if(response.find("已成功拉")!=std::string::npos){
        agree_enter_group(client_fd);
    }
    std::string message="["+client.cur_user+"用户已拉你进入"+group_number+"群聊]("+account+')';
    int online=0;
    for(int j=0;j<account.size();j++){
        if(account[j]==0x05||account[j]==0x06){
            account.erase(j,1);
        }
    }
    if(response.find("已成功")!=std::string::npos){
        for(auto&[fd,a]:clientm){
            if(a.cur_user==account){
                online=1;
                int n=Send(fd,message.c_str(),message.size(),0);
                break;
            }
        }
        if(online==0){
            std::cout<<"has save message"<<std::endl;
            std::string newmes="new message:messages";
            redisReply *mes=(redisReply *)redisCommand(conn,"SADD %s %s",newmes.c_str(),message.c_str());
            freeReplyObject(mes);
        }
    }
    client.group_message.clear();
}

void GroupManager::agree_enter_group(int client_fd){
    auto&client=clientm[client_fd];
    std::cout<<"agree groupmes="<<client.group_message<<std::endl;
    std::string account=client.group_message.substr(0,6);
    std::string group_number=client.group_message.substr(8,9);

    std::string name_key="user:"+account;
    redisReply *getname=(redisReply *)redisCommand(conn,"HGET %s name",name_key.c_str());
    std::string name=getname->str;
    freeReplyObject(getname);

    std::string member=name+" "+account+"(成员)";
    std::string mem_key=group_number+":member";
    redisReply *mem=(redisReply *)redisCommand(conn,"SADD %s %s",mem_key.c_str(),member.c_str());
    freeReplyObject(mem);

    std::string joinkey=account+":joined_groups";
    std::string key="group:"+group_number;
    redisReply *getgname=(redisReply *)redisCommand(conn,"HGET %s group_name",key.c_str());
    std::string group_name=getgname->str;
    freeReplyObject(getgname);
    
    redisReply *getown=(redisReply *)redisCommand(conn,"HGET %s group_owner",key.c_str());
    std::string owner=getown->str;
    freeReplyObject(getown);
    std::string joinmes=group_number+"   "+group_name+" "+owner+("(群主)");
    redisReply *joinown=(redisReply *)redisCommand(conn,"SADD %s %s",joinkey.c_str(),joinmes.c_str());
    freeReplyObject(joinown);
    client.group_message.clear();
}

int GroupManager::send_group_apply_mess(int client_fd,std::string mes){
    int ans=0;
    int pos1=mes.find("聊");
    int pos2=mes.find(']');
    std::string group_number=mes.substr(pos1+3,pos2-pos1-3);
    std::cout<<"send group number="<<group_number<<std::endl;
    std::cout<<"size="<<group_number.size()<<std::endl;
    std::string mem_key=group_number+":member";
    redisReply *getmem=(redisReply *)redisCommand(conn,"SMEMBERS %s",mem_key.c_str());
    std::cout<<"element="<<getmem->elements<<std::endl;
    for(int i=0;i<getmem->elements;i++){
        std::string temp=getmem->element[i]->str;
        std::cout<<"send temp="<<temp<<std::endl;
        if(temp.find("成员")!=std::string::npos||temp.find('(')==std::string::npos){
            continue;
        }
        std::string send_account;
        int pos1=temp.find(' ');
        int pos2=temp.find('(');
        send_account=temp.substr(pos1+1,pos2-pos1-1);
        std::cout<<"send account="<<send_account<<std::endl;
        for(auto&[fd,a]:clientm){
            if(a.cur_user==send_account){
                Send(fd,mes.c_str(),mes.size(),0);
                ans=1;
                break;
            }
        }
    }
    freeReplyObject(getmem);
    return ans;
}

int GroupManager::check_acount_ifexists(int client_fd,std::string acout,std::string group_num){
    auto&client=clientm[client_fd];
    int specail=0;
    if(acout.substr(0,3)=="own"){
        specail=1;
        acout.erase(0,3);
    }
    std::string user_key="user:"+acout;
    int mark=0;
    //账号存在验证
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",user_key.c_str());
    if(reply->integer!=1){
        std::cout<<"账号不存在"<<std::endl;
        mark=1;
    }
    freeReplyObject(reply);
    if(mark!=0) return mark;
    //验证是否操作的账号为自己
    if(specail==0){
        if(client.cur_user==acout){
            std::cout<<"不能对自己进行操作"<<std::endl;
            return -1;
        }
    }
    //群聊账号存在验证
    std::string group_key="group:"+group_num;
    redisReply *ge=(redisReply *)redisCommand(conn,"EXISTS %s",group_key.c_str());
    if(ge->integer!=1){
        std::cout<<"该群聊不存在"<<std::endl;
        mark=2;
    }

    freeReplyObject(ge);
    if(mark!=0) return mark;
    //用户是否属于群聊验证，以及用户身份判断
    std::string member_key=group_num+":member";
    redisReply *ifbelongroup=(redisReply *)redisCommand(conn,"SMEMBERS %s",member_key.c_str());
    int a=0;
    std::string temp;
    for(int i=0;i<ifbelongroup->elements;i++){
        temp=ifbelongroup->element[i]->str;
        std::cout<<"ifbelong temp:"<<temp<<std::endl;
        if(temp.find('(')==std::string::npos) continue;
        //最后账号位数规范以后应进行修改
        if(temp.find(acout)!=std::string::npos&&temp[temp.find('(')-1]==acout[acout.size()-1]){
            a=1;
            break;
        }
        temp.clear();
    }
    freeReplyObject(ifbelongroup);
    if(a==0){
        mark=3;
        return mark;
    }
    if(temp.find("(成员)")!=std::string::npos){
        mark=4;
    }else if(temp.find("(管理员)")!=std::string::npos){
        mark=5;
    }else if(temp.find("(群主)")!=std::string::npos){
        mark=6;
    }
    return mark;
}