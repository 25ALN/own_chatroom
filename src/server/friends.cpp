#include "../include/server/friends.hpp"

friends::friends(redisContext* conn_, 
                 std::unordered_map<int, clientmessage>& clients)
    : conn(conn_), clientm(clients), gm(conn_, clients) // 必须初始化成员
{
}

void friends::show_friends(int client_fd){
    auto &client=clientm[client_fd];
    std::string key=client.cur_user+":friends";
    redisReply *reply=(redisReply*)redisCommand(conn,"SMEMBERS %s",key.c_str()); 
    //SMEMBERS 可获取全部成员
    std::string all_friends;
    std::cout<<"elements="<<reply->elements<<std::endl;
    int cnt=0;
    for(int i=0;i<reply->elements;i++){
        std::string friendsaccount=reply->element[i]->str;
        if(strlen(reply->element[i]->str)<2){
            continue;
        }
        std::string userkey="user:"+friendsaccount;
        redisReply* name=(redisReply*)redisCommand(conn,"HGET %s name",userkey.c_str());
        if(name->type!=REDIS_REPLY_STRING){
            std::cout<<"reddy del"<<friendsaccount<<std::endl;
            redisReply *delf=(redisReply *)redisCommand(conn,"SREM %s %s",key.c_str(),friendsaccount.c_str());
            freeReplyObject(delf);
            freeReplyObject(name);
            continue;
        }
        std::string online;
        int ensure=-1;
        for(auto&[x,a]:clientm){
            if(a.cur_user==friendsaccount){
                ensure=x;
            }
            if(ensure!=-1){
                online="在线";
            }
        }
        if(ensure==-1){
            online="离线";
        } 
        //获取字段名
        if(name&&name->type==REDIS_REPLY_STRING){
            all_friends+=std::to_string(cnt+1)+". "+"name:"+name->str+" account:"+friendsaccount+" "+online+"\n";
            cnt++;
        }else if(!friendsaccount.empty()){
            all_friends+=std::to_string(cnt+1)+". "+"account:"+friendsaccount;
            cnt++;
        }
        freeReplyObject(name);
    }
    if(all_friends.empty()){
        all_friends="你暂时还没有好友";
    }
    freeReplyObject(reply);
    int n=Send(client_fd,all_friends.c_str(),all_friends.size(),0);
    std::cout<<"n="<<n<<std::endl;
    if(n<0){
        perror("show send");
    }    
}

void friends::deal_friends_part(int client_fd,std::string data){
    auto &x=clientm[client_fd];
    if(data.find("(group)")!=std::string::npos&&((data[data.size()-1]==')')||data.find("$?")!=std::string::npos)){
        std::cout<<"group data="<<data<<std::endl;
        x.group_message=data;
    }
    if(data.size()==1&&data=="6"&&x.if_begin_chat!=1){
        x.if_enter_group=1;
    }
    if(data.size()==3&&data=="10t"&&x.if_begin_chat!=1){
        x.if_enter_group=0;
        return;
    }
    int n=data.size();
    std::cout<<"n="<<n<<" friends_part buf="<<data<<std::endl;
    static int mark=0;
    static std::string choose;
    std::string acout;
    if(data.find("identify")!=std::string::npos&&data.size()==17){
        gm.check_identify(client_fd,data);
        return;
    }
    if(data.find("(group)(group owner)")!=std::string::npos){
        gm.own_charger_right(client_fd);
    }else if(data.find("(group)(application)")!=std::string::npos){
        gm.agree_enter_group(client_fd);
    }
    //check new mes 
    std::string key="new message:messages";
    redisReply *reply=(redisReply *)redisCommand(conn,"SMEMBERS %s",key.c_str());
    if(reply->elements>0){
        std::cout<<"        ready send new message"<<std::endl;
        std::string response;
        std::string deltemp;
        for(int i=0;i<reply->elements;i++){
            std::string temp=reply->element[i]->str;
            std::cout<<"temp="<<temp<<std::endl;
            response+=temp;
            if(!response.empty()){
                deltemp=response;
                response+='\n';
                break;
            }
        }
        if(!response.empty()){
            if(response.find("请求加入群聊")!=std::string::npos){
                int n=gm.send_group_apply_mess(client_fd,response);
                if(n==1){
                    std::cout<<"delmes="<<deltemp<<":end"<<std::endl;
                    redisReply *delmes=(redisReply *)redisCommand(conn,"SREM %s %s",key.c_str(),deltemp.c_str());
                    freeReplyObject(delmes);
                }
            }else{
                int pos1=response.find('(');
                int pos2=response.find(')');
                std::string mbaccount=response.substr(pos1+1,pos2-pos1-1);
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
                    int n=Send(target_fd,response.c_str(),response.size(),0);
                    std::cout<<"had send :"<<n<<std::endl;
                    //删除新消息
                    std::string del_key="new message:messages";
                    redisReply *del=(redisReply *)redisCommand(conn,"SREM %s %s",del_key.c_str(),deltemp.c_str());
                    freeReplyObject(del);
                }
            }
        }
    }
    freeReplyObject(reply);

    if(data.find(")refuse")!=std::string::npos&&data.find("被拒绝")!=std::string::npos){
        int pos=data.find('(');
        std::string sendaccount=data.substr(pos+1,6);
        std::string mes="[你的入群申请已被拒绝]("+sendaccount+')';
        for(auto&[fd,a]:clientm){
            if(a.cur_user==sendaccount){
                Send(fd,mes.c_str(),mes.size(),0);
                break;
            }
        }
    }
    if(data.find("agree enter)")!=std::string::npos&&data.size()>20){
        gm.add_user_enterg(client_fd,data);
        return;
    }else if(data.find(")*refuse*")!=std::string::npos){
        int pos=data.find('(');
        std::string account=data.substr(pos+1,6);
        std::string refusemes=data.substr(0,pos);
        for(auto&[fd,a]:clientm){
            if(a.cur_user==account){
                Send(fd,refusemes.c_str(),refusemes.size(),0);
                break;
            }
        }
        return;
    }
    
    if(data.find("ownhmd")!=std::string::npos&&data.size()==12){
        std::string hmdlist;
        std::string hmdac=data.substr(0,6);
        std::cout<<"hmdac="<<hmdac<<std::endl;
        std::string hmdkey=hmdac+":hmd_account";
        redisReply *gethmd=(redisReply *)redisCommand(conn,"SMEMBERS %s",hmdkey.c_str());
        for(int i=0;i<gethmd->elements;i++){
            std::string getname=(std::string)gethmd->element[i]->str;
            std::string pbac=getname;
            getname.insert(0,"user:");
            std::cout<<"getname:"<<getname<<std::endl;
            redisReply *q=(redisReply *)redisCommand(conn,"HGET %s name",getname.c_str());
            hmdlist+=(std::string)q->str+' '+pbac+'\n';
            freeReplyObject(q);
        }
        freeReplyObject(gethmd);
        if(hmdlist.empty()){
            hmdlist="当前还没有用户被你屏蔽";
        }
        hmdlist+="(pblist)";
        Send(client_fd,hmdlist.c_str(),hmdlist.size(),0);
        return;
    }
    if(data=="look past/test/"){
        std::string hiskey=std::to_string(std::stoi(x.chat_with)+std::stoi(x.cur_user))+":history";
        redisReply *lookhistory=(redisReply *)redisCommand(conn,"LRANGE %s 0 -1",hiskey.c_str());
        std::string allmes;
        for(int i=0;i<lookhistory->elements;i++){
            allmes+=lookhistory->element[i]->str;
            allmes+='\n';
        }
        freeReplyObject(lookhistory);
        if(allmes.empty()){
            allmes="当前暂无历史记录";
        }
        Send(client_fd,allmes.c_str(),allmes.size(),0);
    }else if(data.find("glook past")!=std::string::npos){
        std::string his_key=data.substr(0,9)+":ghistory";
        redisReply *lookhis=(redisReply *)redisCommand(conn,"LRANGE %s 0 -1",his_key.c_str());
        std::string allmes;
        for(int i=0;i<lookhis->elements;i++){
            allmes+=lookhis->element[i]->str;
            allmes+='\n';
        }
        freeReplyObject(lookhis);
        if(allmes.empty()){
            allmes="当前暂无历史记录";
        }
        Send(client_fd,allmes.c_str(),allmes.size(),0);
    }
    if(data=="list file/test/"){
        std::string path="/home/aln/桌面/chatroom/downlode/ownfile";
        std::string allfile;
        int pd_change=0;
        for (const auto &entry :std::filesystem::directory_iterator(path)){
            std::string temppath="/home/aln/桌面/chatroom/downlode/ownfile/";
            temppath+=entry.path().filename().string();
            uintmax_t current_size =std::filesystem::file_size(temppath);
            uintmax_t tempsize=current_size;
            for(int i=0;i<=50;i++){
                current_size =std::filesystem::file_size(temppath);
                if(tempsize!=current_size){
                    pd_change=1;
                }
            }
            if(pd_change==0){
                allfile+=entry.path().filename().string();
                allfile+=' ';
            }else{
                pd_change=0;
            }
        }
        if(allfile.empty()){
            allfile="当前还没有可下载的文件";
        }
        Send(client_fd,allfile.c_str(),allfile.size(),0);
    }else if(data.find("glist file$?")!=std::string::npos){
        std::string path="/home/aln/桌面/chatroom/downlode/groupfile";
        std::string allfile;
        int pd_change=0;
        for (const auto &entry :std::filesystem::directory_iterator(path)) {
            uintmax_t current_size =std::filesystem::file_size(entry.path().filename().string());
            uintmax_t tempsize=current_size;
            for(int i=0;i<=50;i++){
                current_size =std::filesystem::file_size(entry.path().filename().string());
                if(tempsize!=current_size){
                    pd_change=1;
                }
            }
            if(pd_change==0){
                allfile+=entry.path().filename().string();
                allfile+=' ';
            }else{
                pd_change=0;
            }
            allfile+=entry.path().filename().string();
            allfile+=' ';
        }
        if(allfile.empty()){
            allfile="当前还没有可下载的文件";
        }
        Send(client_fd,allfile.c_str(),allfile.size(),0);
    }
    if(data.find("need to save")!=std::string::npos){
        int pos=data.find('(');
        int tail=data.find(')');
        x.chat_with=data.substr(0,tail-(tail-pos));
    }
    if(data.find("enter chat state")!=std::string::npos){
        std::cout<<"find data="<<data<<std::endl;
        data=data.substr(sizeof("enter chat state(")-1,data.size()-sizeof("enter chat state()")+1);
        choose="2";
        x.chat_with=data;
        x.if_begin_chat=1;
    }
    if(n>=2){
        acout=data;
    }else if(n==1&&data[0]!=0x05){
        choose=data;
    }
    if(n<0){
        perror("friends recv");
        close(client_fd);
        clientm.erase(client_fd);
        x.login_step=0;
        x.state=0;
        return;
    }else if(n==0){ //说明没有可以读取的数据，直接返回
        x.login_step=0;
        x.state=0;
        return;
    }
    if(n==1&&static_cast<char >(data[0])==0x05){
        x.last_heart_time=time(nullptr);
        const char cmd=0x06;
        Send(client_fd,&cmd,1,0);
        return;
    }
    if(!acout.empty()&&(acout.back()==' ')||(acout.back()=='\n')){
        acout.erase(acout.size()-1);
        std::cout<<"acount_size="<<acout.size()<<std::endl;
    }
    if(data=="7"){
        std::cout<<"客户端"<<client_fd<<"已请求退出"<<std::endl;
        x.login_step=0;
        x.state=0;
        x.cur_user.clear();
        const char *temp="已退出登陆状态";
        int n=Send(client_fd,temp,strlen(temp),0);
        std::cout<<"quit n="<<n<<std::endl;
        return;
    }
    if(acout.empty()&&choose[0]!='1'){
        if(choose[0]=='4'||choose[0]=='2'){
            mark=1;
        }
        return;
    }
    static std::string saveaccount;
    if(mark==1&&!data.empty()&&data.size()>1&&(data.find("同意")==std::string::npos)&&(data.find("拒绝")==std::string::npos)){
        saveaccount=data;
        mark=0;
    }
    // 处理好友请求响应
    static std::string request_account;
    if(!saveaccount.empty()&&(data.find("同意")!=std::string::npos)||(data.find("拒绝")!=std::string::npos)) {
        if(data.back()=='\n'){
            data.erase(data.size()-1);
        }
        if(saveaccount.back()=='\n'){
            saveaccount.erase(saveaccount.size()-1);
        }
        std::cout<<"ready enter cur_user="<<clientm[client_fd].cur_user<<std::endl;
        deal_friends_add_hf(client_fd,data,saveaccount,request_account);
        x.request.clear();
        return;
    }
    //如果没有此处每次传入的账号都是data
    if(x.if_begin_chat==1){
        chat_with_friends(client_fd,x.chat_with,data);
    }
    if(x.if_begin_group_chat==1){
        gm.groups(client_fd);
    }
    if(x.if_enter_group==1){
        gm.groups(client_fd);
        return;
    }
    pool.queuetasks([this,client_fd,x,data,acout]{
    switch (choose[0]){
    case '1':
        show_friends(client_fd);
        break;
    case '2':
        chat_with_friends(client_fd,x.chat_with,data);
        break;
    case '3':
        delete_friends(client_fd,acout);
        break;
    case '4':
        request_account=deal_add_friends(client_fd,acout);
        break;
    case '5':
        ingore_someone(client_fd,acout);
        break;
    case '6':
        gm.groups(client_fd);
        break;
    default:
        break;
    }
    choose.erase();
    });
}

void friends::ingore_someone(int client_fd,std::string account){
    auto &client=clientm[client_fd];
    if(account.back()==' '||account.back()=='\n'){
        account.erase(account.size()-1);
    }
    std::string mark;
    if(account.find("B")!=std::string::npos){
        mark="B";
        int pos=account.find("B");
        account.erase(pos,1);
    }else if(account.find("K")!=std::string::npos){
        mark="K";
        int pos=account.find("K");
        account.erase(pos,1);
    }
    std::string response;
    std::string key="user:"+account;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",key.c_str());
    if(reply->integer!=1){
        response="该账号不存在";
    }else if(mark=="B"){
        std::string friend_key=client.cur_user+":friends";
        redisReply *r=(redisReply *)redisCommand(conn,"SISMEMBER %s %s",friend_key.c_str(),account.c_str());
        if(r->integer!=1){
            response="该用户不是你的好友";
            freeReplyObject(r);
        }else{
            freeReplyObject(r);
            std::string hmd_key=client.cur_user+":hmd_account";
            redisReply *pb=(redisReply *)redisCommand(conn,"SADD %s %s",hmd_key.c_str(),account.c_str());
            if(pb->integer==1){
                response="已将账号为"+account+"的用户屏蔽，你将不会收到任何关于该好友的信息";
            }else if(pb->integer==0){
                response="该好友已被你屏蔽过了";
            }else{
                response="屏蔽失败，异常错误";
            }
            freeReplyObject(pb);
        }
    }else{
        std::string jc_key=client.cur_user+":hmd_account";
        redisReply *jcpb=(redisReply *)redisCommand(conn,"SREM %s %s",jc_key.c_str(),account.c_str());
        if(jcpb&&jcpb->integer==1){
            response="已成功解除对"+account+"用户的屏蔽";
        }else{
            response="解除屏蔽失败";
        }
        freeReplyObject(jcpb);
    }
    freeReplyObject(reply);

    int n=Send(client_fd,response.c_str(),response.size(),0);
    std::cout<<"ingore num="<<n<<std::endl;
    if(n<0){
        perror("ingore send");
        close(client_fd);
        return;
    }
}

void friends::chat_with_friends(int client_fd,std::string account,std::string data){
    auto &client=clientm[client_fd];
    client.chat_with=account;
    std::string key="user:"+account;
    std::string response;
    if(client.if_begin_chat==0){
        redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",key.c_str());
        if(reply->integer!=1){
            response="该账号不存在(no own chat)";
            freeReplyObject(reply);
        }else{
            freeReplyObject(reply);
            if(client.cur_user==account){
                response="不允许与自己私聊!(no own chat)";
            }else{            
                std::string friend_key=client.cur_user+":friends";
                redisReply *r=(redisReply *)redisCommand(conn,"SISMEMBER %s %s",friend_key.c_str(),account.c_str());
                if(r->integer!=1){
                    response="你们还不是好友，不允许进行私聊!(no own chat)";
                }
                freeReplyObject(r);
                if(response.empty()){
                    std::string pb_key=account+":hmd_account";
                    redisReply *pbcz=(redisReply *)redisCommand(conn,"SISMEMBER %s %s",pb_key.c_str(),client.cur_user.c_str());
                    std::string own_pb_key=client.cur_user+":hmd_account";
                    redisReply *ownhmd=(redisReply *)redisCommand(conn,"SISMEMBER %s %s",own_pb_key.c_str(),account.c_str());
                    if(pbcz->integer==1){
                        response="你已被该用户屏蔽(no own chat)";
                    }else if(ownhmd->integer==1){
                        response="该用户已被你屏蔽(no own chat)";
                    }else{
                        int ensure_fd=-1;
                        for(auto&[fd,x]:clientm){
                            if(x.cur_user==account){
                                ensure_fd=fd;
                                break;
                            }
                        }
                        if(ensure_fd!=-1){
                            response="可以向该好友发送消息了 (在线)";
                        }else{
                            response="可以向该好友发送消息了 (离线)";
                        }
                        client.if_begin_chat=1;
                    }
                    freeReplyObject(pbcz);
                    freeReplyObject(ownhmd);
                }
            }
        }
    }else if(data.find("/test/")!=std::string::npos){
            std::string tempmes=data.substr(0,data.find("/test/"));
            std::string own_key="user:"+client.cur_user;
            redisReply *getname=(redisReply *)redisCommand(conn,"HGET %s name",own_key.c_str());
            std::string mh="]:";
            std::string own_name="[";
            own_name+=getname->str;
            own_name+=mh;
            freeReplyObject(getname);
            std::string other_name;
            int chat_people=-1;
            int if_enter_chat=0;
            for(auto &[fd,a]:clientm){
                if(a.cur_user==account){
                    chat_people=fd;
                    if_enter_chat=a.if_begin_chat;
                    break;
                }
            }
            tempmes.insert(0,own_name);
            int n=0;
            //验证是否突然被删或被屏蔽
            std::string willsendmes;
            static std::mutex checklock;
            {
                std::lock_guard <std::mutex> checkLock(checklock);
                int mark_specail=0;
                std::string checkpb_key=account+":hmd_account";
                redisReply *cpbcz=(redisReply *)redisCommand(conn,"SISMEMBER %s %s",checkpb_key.c_str(),client.cur_user.c_str());
                if(cpbcz&&cpbcz->type==REDIS_REPLY_INTEGER&&cpbcz->integer==1){
                    mark_specail=1;
                }
                freeReplyObject(cpbcz);
                
                std::string friend_key=client.cur_user+":friends";
                redisReply *r=(redisReply *)redisCommand(conn,"SISMEMBER %s %s",friend_key.c_str(),account.c_str());
                if(r && r->type == REDIS_REPLY_INTEGER&&r->integer!=1){
                    mark_specail=2;
                }
                freeReplyObject(r);
                willsendmes=tempmes;
                if(mark_specail==1||mark_specail==2){
                    if(mark_specail==1){
                        willsendmes="你已被对方屏蔽，无法继续发送消息";
                    }else if(mark_specail==2){
                        willsendmes="你已被对方删除，无法继续发送消息";
                    }
                    recv_buffer.clear();
                    std::string delmes=willsendmes+" ("+client.cur_user+")0x01";
                    if(client.if_begin_chat==1){
                        Send(client_fd,delmes.c_str(),delmes.size(),0);
                    }
                    client.if_begin_chat=0;
                }
            }
            std::string savemess=tempmes;
            if(chat_people!=-1){
                willsendmes+=" ("+account+")0x01";
                if(if_enter_chat==0){
                    willsendmes=own_name+"用户向你发送了一条私聊消息";
                }
                int tempn=Send(chat_people,willsendmes.c_str(),willsendmes.size(),0);
                std::cout<<"chat n="<<tempn<<std::endl;
            }
            if(chat_people==-1||(n>0&&data.find("/exit")==std::string::npos)){
                long long key_num=std::stoll(account)+std::stoll(client.cur_user);
                std::string his_key=std::to_string(key_num)+":history";
                int pos=savemess.find(':');
                pos+=1;
                if(savemess.substr(pos,4)=="STOR"||savemess.substr(pos,4)=="RETR"){
                    if(savemess.substr(pos,4)=="STOR"){
                        savemess.erase(pos,4);
                        savemess.insert(pos,"上传文件");
                    }else{
                        savemess.erase(pos,4);
                        savemess.insert(pos,"下载文件");
                    }
                }
                redisReply *history=(redisReply *)redisCommand(conn,"RPUSH %s %s",his_key.c_str(),savemess.c_str());
                freeReplyObject(history);
        }
        data.clear();
    }
    if(!response.empty()){
        //发出前50条聊天记录
        std::string history_key=std::to_string(std::stoi(account)+std::stoi(client.cur_user))+":history";
        std::string chatmes="最近的前50条聊天记录\n";
        redisReply *gethistory=(redisReply *)redisCommand(conn,"LRANGE %s -50 -1",history_key.c_str());
        if(gethistory->type==REDIS_REPLY_ARRAY){
            for(int i=0;i<gethistory->elements;i++){
                chatmes+=gethistory->element[i]->str;
                chatmes+='\n';
            }
        }
        freeReplyObject(gethistory);
        if(response.find("可以向")!=std::string::npos){
            response.insert(0,chatmes);
        }
        int n=Send(client_fd,response.c_str(),response.size(),0);
        if(n<0){
            perror("chat send");
            close(client_fd);
        }else{
            std::cout<<"respons:"<<response<<std::endl;
        }
    }
}

void friends::delete_friends(int client_fd,std::string account){
    auto &client = clientm[client_fd];
    std::string response;
    std::string key = client.cur_user+":friends";

    // 判断好友集合是否存在
    redisReply *reply=(redisReply *)redisCommand(conn, "EXISTS %s", key.c_str());
    std::cout<<"key="<<key<<std::endl;

    if(!reply||reply->type != REDIS_REPLY_INTEGER||reply->integer != 1) {
        response="你还没有添加任何好友！";
        freeReplyObject(reply);
    }else{
        freeReplyObject(reply); 
        // 获取好友列表
        redisReply *del = (redisReply *)redisCommand(conn, "SMEMBERS %s", key.c_str());
        if (!del || del->type != REDIS_REPLY_ARRAY) {
            response = "获取好友列表失败！";
            if (del) freeReplyObject(del);
        } else {
            bool found=false;
            for (int i=0;i<del->elements;i++) {
                std::string temp=del->element[i]->str;
                if (temp.find(account)!=std::string::npos) { // 找到要删除的好友
                    redisReply *deal_ys=(redisReply *)redisCommand(conn, "SREM %s %s", key.c_str(), temp.c_str());
                    std::string a=temp+":friends";
                    //对方的好友列表里面也不能有你自己了
                    redisReply *other_del_you=(redisReply *)redisCommand(conn,"SREM %s %s",a.c_str(),client.cur_user.c_str());
                    if(deal_ys&&deal_ys->integer==1) {
                        response="已成功删除账号为"+account+"的好友";
                        std::string hmd_key=client.cur_user+":hmd_account";
                        redisReply *delhmd=(redisReply *)redisCommand(conn,"DEL %s",hmd_key.c_str());
                        freeReplyObject(delhmd);
                        for(auto&[fd,a]:clientm){
                            if(a.cur_user==account&&a.if_begin_chat==1){
                                std::string mes="你已被对方删除请输入/exit后退出私聊";
                                mes+=0x07;
                                Send(fd,mes.c_str(),mes.size(),0);
                                break;
                            }
                        }
                    }else{
                        response="删除失败，出现异常错误";
                    }
                    if(deal_ys) freeReplyObject(deal_ys);
                    if(other_del_you) freeReplyObject(other_del_you);
                    found=true;
                    break;
                }
            }
            if(found){
                long long temp=std::stoll(account)+std::stoll(client.cur_user);
                std::string history_key=std::to_string(temp)+":history";
                redisReply *delhistory=(redisReply *)redisCommand(conn,"DEL %s",history_key.c_str());
                freeReplyObject(delhistory);
            }
        if(!found){
            response="该用户不是你的好友！";
        }
        freeReplyObject(del);
        }
    }
    int n=Send(client_fd,response.c_str(),response.size(),0);
    if(n<0){
        perror("delete send");
        close(client_fd);
    }
}

std::string friends::deal_add_friends(int client_fd,std::string account){
    
    if (!account.empty()&&account.back()=='\n') {
        account.erase(account.size()-1);
    }
    auto &client=clientm[client_fd];
    std::cout<<"enter add friends"<<std::endl;
    std::cout<<"user:"<<client.cur_user<<std::endl;
    std::cout<<"account="<<account<<std::endl;
    // 检查是否添加自己
    if(client.cur_user==account){
        std::string temp="不能添加自己为好友!";
        Send(client_fd,temp.c_str(),temp.size(),0);
        return "0";
    }
    // 检查账号是否存在
    std::string userKey="user:"+account;
    redisReply *reply=(redisReply *)redisCommand(conn,"EXISTS %s",userKey.c_str());
    if (reply->integer!=1){
        std::string temp="该账号不存在！";
        Send(client_fd,temp.c_str(),temp.size(),0);
        freeReplyObject(reply);
        return "0";
    }
    freeReplyObject(reply);
    
    // 检查是否已是好友
    std::string friendsKey=client.cur_user+":friends";
    reply=(redisReply*)redisCommand(conn,"SISMEMBER %s %s",friendsKey.c_str(), account.c_str());
    if (reply->integer==1){
        std::string temp="你们已经是好友了！";
        Send(client_fd,temp.c_str(),temp.size(), 0);
        freeReplyObject(reply);
        return "0";
    }
    freeReplyObject(reply);
    
    //检查是否被屏蔽
    std::string pb_key=account+":hmd_account";
    redisReply *pbcz=(redisReply *)redisCommand(conn,"SISMEMBER %s %s",pb_key.c_str(),account.c_str());
    if(pbcz->integer==1){
        std::string temp="你已被该用户屏蔽";
        Send(client_fd,temp.c_str(),temp.size(),0);
        freeReplyObject(pbcz);
        return "0";
    }
    freeReplyObject(pbcz);
    // 查找目标用户是否在线
    bool found=false;
    for(auto& [fd, cli]:clientm) {
        if (cli.cur_user==account) {
            std::string msg="好友请求:"+client.cur_user+"请求添加你为好友";
            msg.insert(0,"[");
            msg+=']';
            msg+='('+account+')';
            int n=Send(fd,msg.c_str(),msg.size(),0);
            std::cout<<"online n="<<n<<std::endl;
            found=true;
            break;
        }
    }
    std::string mesa=found?"请求已发送":"对方不在线，请求已保存";
    if(!found){
        std::cout<<"ready send disonline mes"<<std::endl;
        std::string newmes_key="new message:messages";
        std::string mes="好友请求:"+client.cur_user+"请求添加你为好友";
        mes.insert(0,"[");
        mes+=']';
        mes+='('+account+')';
        redisReply *newmes=(redisReply *)redisCommand(conn,"SADD %s %s",newmes_key.c_str(),mes.c_str());
        freeReplyObject(newmes);
    }
    Send(client_fd, mesa.c_str(), mesa.size(), 0);
    return client.cur_user;
}

void friends::deal_friends_add_hf(int client_fd, const std::string& response, const std::string& requester,std::string own_account) {
    std::cout<<"response="<<response<<" requestr="<<requester<<std::endl;
    auto& client = clientm[client_fd];
    
    if (response.find("同意")!=std::string::npos) {
        std::string myFriends=own_account+":friends";

        std::string theirFriends = requester +":friends";
        std::cout<<"cur_user="<<own_account<<":end"<<std::endl;
        std::cout<<"myfriends="<<myFriends<<":end"<<std::endl;
        std::cout<<"theirFriends="<<theirFriends<<":end"<<std::endl;
        redisCommand(conn, "SADD %s %s", myFriends.c_str(), requester.c_str());
        redisCommand(conn, "SADD %s %s", theirFriends.c_str(), own_account.c_str());

        std::string msg="[你现在和"+own_account+"用户是好友了!]";
        msg+=0x07;
        Send(client_fd,msg.c_str(),msg.size(), 0);
        //向对方发送好友消息
        int if_online=0;
        for(auto&[fd,a]:clientm) {
            if (a.cur_user==own_account){
                if_online=1;
                std::string message="[你现在和"+client.cur_user+"用户是好友了!]("+own_account+')';
                Send(fd,message.c_str(),message.size(),0);
                break;
            }
        }
        if(if_online==0){
            std::string key="new message:messages";
            std::string msee="[你们现在是好友了]("+own_account+')';
            redisReply *reply=(redisReply *)redisCommand(conn,"SADD %s %s",key.c_str(),msg.c_str());
            freeReplyObject(reply);
        }
    }else{
        std::string msg="已拒绝对方的好友申请";
        msg+=0x07;
        int n=Send(client_fd, msg.c_str(), msg.size(), 0);
        std::cout<<"refuse mess n="<<n<<std::endl;
        if(n<0){
            perror("hf send");
        }
        int if_online=0;
        for(auto&[fd,a]:clientm) {
            if (a.cur_user==own_account){
                if_online=1;
                std::string message="[好友申请被拒绝]("+own_account+')';
                message+=0x07;
                Send(fd,message.c_str(),message.size(),0);
                std::cout<<"df refuse mess"<<std::endl;
                break;
            }
        }
        if(if_online==0){
            std::string key="new message:messages";
            std::string mess="[好友申请被拒绝](";
            mess+=own_account+')';
            redisReply *reply=(redisReply *)redisCommand(conn,"SADD %s %s",key.c_str(),mess.c_str());
            freeReplyObject(reply);
        }
    }
}