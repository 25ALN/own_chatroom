#include "../ftpser.hpp"

int fsRecv(int fd,char *buf,int len,int flag);
int fsSend(int fd,char *buf,int len,int flag);
const int first_port=2100;
int mes_travel_port=5100;
const int maxevents=100;

class client_data{
    public:
    int client_fd;  //控制连接所用的描述符
    int listen_fd;   //listen描述符
    int data_fd;    //数据连接所用的描述符
    bool pasv_flag; //pasv模式是否打开的标志
    std::string server_ip;

    client_data(int fd,const std::string &ip):client_fd(fd),listen_fd(-1),data_fd(-1),pasv_flag(false)
    ,server_ip(ip){}
};
std::unordered_map<int,std::shared_ptr <client_data> > client_message; //利用哈希表来从将信息一一对应起来
class ftpserver{
    public:
    void start();
    ~ftpserver();

    private:
    
    int connect_init();
    void error_report(const std::string &x,int fd); //错误处理
    void ser_work(int fd,std::string ip);
    void clean_connect(int fd);

    void deal_new_connect(int ser_fd,int epoll_fd);
    void deal_pasv_data(int clinet_fd);
    void deal_STOR_data(std::shared_ptr<client_data> client,std::string filename);
    void deal_RETR_data(std::shared_ptr<client_data> client,std::string filename);
    void deal_client_data(int data_fd);
};

void ftpserver::start(){
    int readyf=0; //记录准备好事件的变量
    int server_fd=connect_init();
    int epoll_fd=epoll_create(1);

    int flags=fcntl(server_fd,F_GETFL,0);
    fcntl(server_fd,F_SETFL,flags|O_NONBLOCK);

    struct epoll_event ev,events[maxevents];
    ev.data.fd=server_fd;
    ev.events=EPOLLIN|EPOLLET;
    epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev);
    
    while(true){
        readyf=epoll_wait(epoll_fd,events,maxevents,-1);
        if(readyf<0){
            perror("epoll_wait");
            break;
        }
        for(int i=0;i<readyf;i++){
            struct sockaddr_in client_mes;
            if(events[i].data.fd==server_fd){ //处理
                deal_new_connect(server_fd,epoll_fd);
            }else if(events[i].events&EPOLLIN){ //接受消息
                deal_client_data(events[i].data.fd);
            }else if(events[i].events&(EPOLLERR|EPOLLHUP)){
                clean_connect(events[i].data.fd);
            }
        }
    }
    for(auto&[fd,client]:client_message){
        clean_connect(fd);
    }
    shutdown(server_fd,SHUT_RDWR);
    close(server_fd);
    close(epoll_fd);
}

void ftpserver::error_report(const std::string &x,int fd){
    std::cout<<x<<" start fail"<<std::endl;
    close(fd);
}

int ftpserver::connect_init(){
    int fd=socket(AF_INET,SOCK_STREAM,0);
    int contain;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&contain,sizeof(int)); //设置端口复用
    struct sockaddr_in ser;
    ser.sin_family=AF_INET;
    ser.sin_port=htons(first_port);
    ser.sin_addr.s_addr=htonl(INADDR_ANY);
    std::string serip="0.0.0.0";
    //ser.sin_addr.s_addr=htonl(serip.c_str());
    int bind_fd=bind(fd,(struct sockaddr*)&ser,sizeof(ser));
    if(bind_fd<0){
        error_report("bind",fd);
    }
    int listen_fd=listen(fd,5);
    if(listen_fd<0){
        error_report("listen",fd);
    }
    return fd;
}

void ftpserver::deal_new_connect(int ser_fd,int epoll_fd){
    std::cout<<"    ftp have new message"<<std::endl;
    struct sockaddr_in client_mes;
    socklen_t mes_len = sizeof(client_mes);
    while (true) {
        int client_fd = accept(ser_fd, (struct sockaddr*)&client_mes, &mes_len);
        if (client_fd < 0) {
            if (errno == EAGAIN||errno == EWOULDBLOCK) break;
            else error_report("accept", client_fd);
        }
        // 获取服务端本地IP地址
        struct sockaddr_in server_side_addr;
        socklen_t addr_len = sizeof(server_side_addr);
        getsockname(client_fd, (struct sockaddr*)&server_side_addr, &addr_len);
        std::string server_ip = inet_ntoa(server_side_addr.sin_addr);
        std::string client_ip=inet_ntoa(client_mes.sin_addr);
        auto client = std::make_shared<client_data>(client_fd, client_ip);
        client->server_ip=server_ip; 
        client_message[client_fd] = client;
        int flags=fcntl(client_fd,F_GETFL,O_NONBLOCK);
        fcntl(client_fd,F_SETFL,flags|O_NONBLOCK);
        struct epoll_event cev;
        
        cev.data.fd=client_fd,
        cev.events=EPOLLET|EPOLLIN;
        
        epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&cev);
    }
}

void ftpserver::deal_client_data(int data_fd){
    char ensure[1024];
    memset(ensure,'\0',sizeof(ensure));
    int n=fsRecv(data_fd,ensure,sizeof(ensure),0);
    while(n<=0){
        fsRecv(data_fd,ensure,sizeof(ensure),0);
    }
    if(n<0){
        std::cout<<"recv error"<<std::endl;
        close(data_fd);
        return;
    }
    std::string command(ensure, n);
    size_t crlf_pos = command.find("\r\n");
    if (crlf_pos != std::string::npos) {
        command = command.substr(0, crlf_pos); // 去除尾部 \r\n
    }
    if(!command.empty()){ //将首尾的空格去除
        command.erase(0,command.find_first_not_of(" "));
        command.erase(command.find_last_not_of(" ")+1);
    }
    auto client=client_message[data_fd];
    if(command.find("PASV")!=std::string::npos){
        deal_pasv_data(data_fd);
    }else if(command.find("STOR")!=std::string::npos){
        std::cout<<"begin STOR"<<std::endl;
        deal_STOR_data(client,command); //从第六个字节开始读取文件名称
    }else if(command.find("RETR")!=std::string::npos){
        std::cout<<"begin RETR"<<std::endl;
        deal_RETR_data(client,command);
    }else if(command.find("quit")!=std::string::npos){
        return;
    }
    else{
        std::cout<<"没有找到相关命令"<<std::endl;
        return;
    }
}

void ftpserver::deal_pasv_data(int client_fd){
    
    auto client = client_message[client_fd];
    std::string server_ip = client->server_ip; // 使用保存的服务端IP
    //创建监听套接字
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return;
    }
    int flags=fcntl(listen_fd,F_GETFL,0);
    fcntl(listen_fd,F_SETFL,flags|O_NONBLOCK); 
    // 设置端口复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0); // 随机分配一个端口
    inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return;
    }
    socklen_t addrlen=sizeof(addr);
    getsockname(listen_fd,(struct sockaddr*)&addr,&addrlen);
    mes_travel_port=ntohs(addr.sin_port);
    if (listen(listen_fd, 1) < 0) {
        perror("listen");
        close(listen_fd);
        return;
    }
    client->listen_fd = listen_fd;
    
    std::vector<std::string> call_mes;
    std::istringstream iss(server_ip);
    std::string part;
    while (getline(iss, part, '.')) {
        call_mes.push_back(part);
    }
    //补充后面两位
    std::ostringstream bc;
    bc<<"227 Entering Passive Mode (";
    for(int i=0;i<call_mes.size();i++){
        bc << call_mes[i];
        if (i != call_mes.size() - 1) bc << ",";
    }
    bc<<","<<(mes_travel_port/256)<<","<<(mes_travel_port%256)<<")\r\n";
    std::string mesg=bc.str();
    std::cout<<mesg<<std::endl;
    int x=fsSend(client_fd,const_cast<char *>(mesg.c_str()),strlen(mesg.c_str()),0);
    if(x<=0){
        error_report("send",client_fd);
        return;
    }
}

void ftpserver::deal_RETR_data(std::shared_ptr<client_data> client,std::string filename){
    std::string response = "150 Opening data connection\r\n";
    fsSend(client->client_fd, const_cast<char*>(response.c_str()), response.size(), 0);
    struct sockaddr_in clmes;
    socklen_t len=sizeof(clmes);
    int data_fd=-1;
    while (true) {
        data_fd = accept(client->listen_fd, (struct sockaddr*)&clmes, &len);
        if (data_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞模式下无连接，稍后重试
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                perror("accept error");
                return;
            }
        } else {
            break;
        }
    }
    if(data_fd<0){
        perror("accept data connection");
        return;
    }
    client->data_fd=data_fd;

    std::string savemark=filename.substr(0,3);
    filename.erase(0,8);
    std::string filepath;
    if(savemark=="own"){
        const char* dir = "./../downlode/ownfile"; 
        char abs_path[PATH_MAX];
        if (realpath(dir, abs_path) != NULL) {
        } else {
            perror("realpath failed");
        }
        filepath=abs_path;
    }else{
        const char* dir = "./../downlode/groupfile"; 
        char abs_path[PATH_MAX];
        if (realpath(dir, abs_path) != NULL) {
        } else {
            perror("realpath failed");
        }
        filepath=abs_path;
    }
    std::string x=filepath+"/"+filename;
    if(x.find("\r\n")!=std::string::npos){
        int pos=x.find("\r\n");
        x=x.substr(0,pos);
    }else if(x.find('\n')!=std::string::npos){
        int pos=x.find('\n');
        x=x.substr(0,pos);
    }
    std::cout<<"x="<<x<<std::endl;
    int file_fd=open(x.c_str(),O_RDONLY);
    if(file_fd < 0){
        perror("open");
        return;
    }
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) < 0) {
        perror("fstat");
        close(file_fd);
        return;
    }
    off_t offset = 0;
    size_t total_size = file_stat.st_size;
    std::cout <<"开始上传文件"<< std::endl;

    while (offset < total_size) {
        ssize_t sent = sendfile(data_fd, file_fd, &offset, total_size - offset);
        if (sent > 0) {
        } else if (sent == 0) {
            break;
        } else {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            } else {
                perror("sendfile error");
                break;
            }
        }
    }
    std::cout << "文件上传完毕" << std::endl;
    close(file_fd);
    shutdown(client->data_fd,SHUT_RDWR);
    close(data_fd);
    client->data_fd=-1;
}

void ftpserver::deal_STOR_data(std::shared_ptr<client_data> client, std::string filename) {
    std::string savemark=filename.substr(0,3);
    filename.erase(0,8);
    // 1. 发送150响应
    std::string response = "150 Opening data connection\r\n";
    fsSend(client->client_fd, const_cast<char*>(response.c_str()), response.size(), 0);
    // 2. 接受数据连接（确保非阻塞模式正确处理）
    int data_fd = -1;
    struct sockaddr_in clmes;
    socklen_t len = sizeof(clmes);
    if(filename.find('\n')!=std::string::npos){
        int pos=filename.find('\n');
        filename=filename.substr(0,pos);
    }
    for(int i=0;i<filename.size();i++){
        if(filename[i]=='/'){
            filename.erase(i,1);
        }
    }
    while (true) {
        data_fd = accept(client->listen_fd, (struct sockaddr*)&clmes, &len);
        if (data_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 非阻塞模式下无连接，稍后重试
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                perror("accept error");
                return;
            }
        } else {
            break;
        }
    }
    client->data_fd = data_fd;
    std::string filepath;
    if(savemark=="own"){
        const char* dir = "./../downlode/ownfile"; 
        char abs_path[PATH_MAX];
        if (realpath(dir, abs_path) != NULL) {
        } else {
            perror("realpath failed");
        }
        filepath=abs_path;
    }else{
        const char* dir = "./../downlode/groupfile"; 
        char abs_path[PATH_MAX];
        if (realpath(dir, abs_path) != NULL) {
        } else {
            perror("realpath failed");
        }
        filepath=abs_path;
    }
    filepath+='/'+filename;
    // 3. 接收文件数据
    std::cout<<"filepath:"<<filepath<<std::endl;
    FILE* fp = fopen(filepath.c_str(), "wb");
    if (!fp) {
        perror("fopen failed");
        close(data_fd);
        return;
    }
    ssize_t total = 0;
    while (true) {
        char buf[131072];
        ssize_t n = recv(data_fd, buf, sizeof(buf), 0);
        if (n > 0) {
            fwrite(buf, 1, n, fp);
            total += n;
        } else if (errno == EAGAIN) {
            continue; 
        } else if (n == 0) {
            break; 
        } else {
            perror("recv error");
            break;
        }
    }
    std::cout<<"total="<<total<<std::endl;
    std::cout<<"文件接收完毕"<<std::endl;
    fclose(fp);
    close(data_fd);
    client->data_fd = -1;
}

void ftpserver::clean_connect(int fd){
    if(client_message.count(fd)){
        auto &client=client_message[fd];
        if(client->listen_fd!=-1){
            shutdown(client->listen_fd,SHUT_RDWR);  //先调用这个可以防止资源泄漏，数据为还未传输完就close会导致数据一直阻塞下去
            close(client->listen_fd);
            client->listen_fd=-1;
        }
        if(client->data_fd!=-1){
            shutdown(client->data_fd,SHUT_RDWR);
            close(client->data_fd);
            client->data_fd=-1;
        }
        client_message.erase(fd); //从哈希表中移除
    }
    shutdown(fd,SHUT_RDWR);
    close(fd);
}

ftpserver::~ftpserver(){
    for(auto&[fd,client]:client_message){
        if(client->listen_fd!=-1){
            shutdown(client->listen_fd,SHUT_RDWR);  //先调用这个可以防止资源泄漏，数据为还未传输完就close会导致数据一直阻塞下去
            close(client->listen_fd);
            client->listen_fd=-1;
        }
        if(client->data_fd!=-1){
            shutdown(client->data_fd,SHUT_RDWR);
            close(client->data_fd);
            client->data_fd=-1;
        }
        client_message.erase(fd); //从哈希表中移除
        shutdown(fd,SHUT_RDWR);
        close(fd);
    }
}

int fsSend(int fd,char *buf,int len,int flag){
    int reallen=0;
    while(reallen<len){
        int temp=send(fd,buf+reallen,len-reallen,flag);
        if(temp<0){
            perror("send");
            close(fd);
            break;
        }else if(temp==0){
            //数据已全部发送完毕
            break;
        }
        reallen+=temp;
    }
    return reallen;
}

int fsRecv(int fd,char *buf,int len,int flag){
    int reallen=0;
    while(reallen<len){
        int temp = recv(fd, buf + reallen, len - reallen, flag);
        if (temp > 0) {
            reallen += temp;
        } else if (temp == 0) { // 连接关闭
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 非阻塞模式下无更多数据，退出循环
            }else{
                perror("recv");
                return -1;
            }
        }
    }
    return reallen;
}