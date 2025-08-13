#include "../ftpcli.hpp"

int fcRecv(int fd,char *buf,int len,int flags);
int fcSend(int fd,const char *buf,int len,int flags);
void error_report(const std::string &x,int fd);
static int mark=1;
class ftpclient{
    public:
    void start(std::string message,std::string serip);
    ~ftpclient();
    
    private:

    const int first_port=2100;
    int data_port=-1;
    std::string IP;
    const int maxt=1000;
    int client_fd=-1;

    void connect_init();
    void start_PASV_mode(int fd,std::string first_m);
    void get_ip_port(std::string ser_mesage);
    void deal_willsend_message(int fd,char m[1024]);
    void deal_send_message(int fd,std::string m);
    void deal_get_file(std::string filename,int fd);
    void deal_up_file(std::string filename,int fd);
};

void ftpclient::start(std::string message,std::string serip){
    IP=serip;
    connect_init();
    if (client_fd<0){
        error_report("socket", client_fd);
    }
    start_PASV_mode(client_fd, "PASV\r\n");
    char repose[1024];
    memset(repose, '\0', sizeof(repose));
    int n=fcRecv(client_fd,repose,sizeof(repose),0);
    while(n<=0){
        fcRecv(client_fd,repose,sizeof(repose),0);
    }
    if (n<=0){
        perror("init recv");
        return;
    }
    std::string cmd(repose, n);
    if (cmd.find("227 Entering Passive Mode") != std::string::npos){
        get_ip_port(cmd);
    }
    else{
        error_report("PASV mode", client_fd);
    }

    //new change
    deal_willsend_message(client_fd,message.data());
    
    while(mark==0);
    deal_willsend_message(client_fd,"quit");
    sleep(1);
    mark=0;
    return;
}

void ftpclient::start_PASV_mode(int fd,std::string first_m){
    sleep(1);
    if (!first_m.empty()){
        int n=fcSend(fd, first_m.c_str(), first_m.size(), 0);
        if(n<0){
            perror("start pasv send");
        }
    }
}
void ftpclient::deal_willsend_message(int fd, char m[1024]){
    std::string mes = (std::string)m;
    if(mes.find("PASV") != std::string::npos){
        std::thread x([this,fd,mes](){
            deal_send_message(fd, mes);
        });
        x.detach();
    }else if (mes.find("STOR") != std::string::npos){
        std::string savemakr=mes.substr(0,3);
        mes.erase(0,3);
        std::string stor_command = "STOR " + mes.substr(5)+"\r\n";
        stor_command.insert(0,savemakr);
        fcSend(fd, stor_command.c_str(), stor_command.size(), 0);
        // 启动线程处理文件传输
        std::thread xo([this,fd, mes](){
            deal_up_file(mes.substr(5), fd);
        });
        xo.detach();
    }else if (mes.find("RETR") != std::string::npos){
        std::string savemakr=mes.substr(0,3);
        mes.erase(0,3);
        std::string retr_command = "RETR " + mes.substr(5) + "\r\n";
        retr_command.insert(0,savemakr);
        fcSend(fd, retr_command.c_str(), retr_command.size(), 0);
        std::thread xo([this,fd, mes](){ 
            deal_get_file(mes.substr(5), fd);
        });
        xo.detach();
    }else if (mes.find("quit") != std::string::npos){
        std::thread xo([this,fd,mes](){
            deal_send_message(fd,mes);
        });
        xo.detach();
        sleep(1);
        return;
    }else{
        std::cout << "not find this command" << std::endl;
        return;
    }
}

void ftpclient::deal_send_message(int fd, std::string m){
    if(!m.empty()){
        fcSend(fd, m.c_str(), m.size(), 0);
    }
}

void ftpclient::connect_init(){
    client_fd=socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd<0){
        error_report("socket",client_fd);
    }
    struct sockaddr_in client_mess;
    client_mess.sin_family = AF_INET;
    client_mess.sin_port = htons(first_port);
    // client_mess.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_pton(AF_INET, IP.c_str(), &client_mess.sin_addr);
    int flags = fcntl(client_fd,F_GETFL,0);
    if (fcntl(client_fd,F_SETFL,flags|O_NONBLOCK) < 0){
        error_report("fcntl",client_fd);
    }
    int connect_fd=connect(client_fd,(struct sockaddr *)&client_mess,sizeof(client_mess));
    if (connect_fd < 0&&errno!=EINPROGRESS){
        error_report("connect",client_fd);
    }
}

void ftpclient::get_ip_port(std::string ser_mesage){
    std::string ip;
    int cnt = 0;
    int start = ser_mesage.find("(");
    int end = ser_mesage.find(")");
    std::string mes = ser_mesage.substr(start + 1, end - start - 1);
    int douhao = mes.find(",");
    std::string p1, p2;
    while (cnt != 4){
        for (int i=0;i<douhao;i++){
            ip += mes[i];
        }
        if(cnt!=3){
            ip+='.';
            mes.erase(0,douhao+1);
            douhao=mes.find(",");
        }
        cnt++;
    }
    mes.erase(0,douhao+1);
    for(auto j:mes){
        if (j!=','&&cnt!=4){
            p1+=j;
        }
        else if(j!=')'&&j!=','){
            p2+=j;
        }
        else if(j==','){
            cnt++;
        }
    }
    IP=ip;
    data_port=std::stoi(p2)*256+std::stoi(p1);
}

void ftpclient::deal_get_file(std::string filename,int fd){
    int data_fd=socket(AF_INET,SOCK_STREAM,0);
    int flag=fcntl(data_fd,F_GETFL,0);
    fcntl(data_fd,F_SETFL,flag|O_NONBLOCK);
    if (data_fd<0){
        error_report("socket", data_fd);
    }
    struct sockaddr_in filedata;
    filedata.sin_family=AF_INET;
    filedata.sin_port=htons(data_port);
    if(IP.empty()){
        return;
    }
    if(inet_pton(AF_INET,IP.c_str(),&filedata.sin_addr)<0){
        std::cout<<"inet_pton fail"<<std::endl;
        return;
    }
    int m=connect(data_fd,(struct sockaddr *)&filedata,sizeof(filedata));
    if(m<0&&errno!=EINPROGRESS){
        std::cout<<"connect fail" << std::endl;
        return;
    }
    // 使用select等待连接完成
    fd_set set;
    FD_ZERO(&set);
    FD_SET(data_fd, &set);
    struct timeval timeout;
    timeout.tv_sec=2;
    timeout.tv_usec=0;
    if(select(data_fd+1,NULL,&set,NULL,&timeout)<=0){
        std::cout << "Connection timeout" << std::endl;
        close(data_fd);
        return;
    }
    if(filename.empty()){
        return;
    }
    if(filename.find('\n')!=std::string::npos){
        int pos=filename.find('\n');
        filename=filename.substr(0,pos);
    }
    char c[1024];
    memset(c,'\0',sizeof(c));
    getcwd(c,sizeof(c));
    std::string path=c;
    path.erase(path.find("bin"),3);
    path+="downlodetest/";
    int n=mkdir(path.c_str(),0755);
    path+=filename;
    FILE *fp=fopen(path.c_str(),"wb");

    if(fp==nullptr){
        std::cout<<"file open fail"<< std::endl;
        shutdown(fd, SHUT_RDWR);
        close(data_fd);
        fclose(fp);
        return;
    }
    std::cout<<"开始接收文件"<<std::endl;
    while(true){
        char buf[131072];
        ssize_t n=recv(data_fd, buf, sizeof(buf), 0);
        if (n > 0) {
            fwrite(buf, 1, n, fp);
        } else if(n == 0) {
            break; // 连接关闭
        } else if(errno==EAGAIN) {
            continue; // 非阻塞模式下重试
        } else {
            perror("recv error");
            break;
        }
    }
    std::cout<<"文件接收完毕"<<std::endl;
    shutdown(data_fd, SHUT_RDWR);
    close(data_fd);
    fclose(fp);
}

void ftpclient::deal_up_file(std::string filename, int control_fd)
{
    // 1. 创建数据套接字并连接到服务端数据端口
    int data_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (data_fd < 0){
        error_report("socket", data_fd);
        return;
    }
    // 设置为非阻塞模式
    int flags = fcntl(data_fd, F_GETFL, 0);
    fcntl(data_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in data_addr;
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(data_port);
    inet_pton(AF_INET, IP.c_str(), &data_addr.sin_addr);
    // 发起非阻塞连接
    int connect_ret = connect(data_fd, (struct sockaddr *)&data_addr, sizeof(data_addr));
    if (connect_ret < 0 && errno != EINPROGRESS)
    {
        error_report("connect", data_fd);
        close(data_fd);
        return;
    }

    //使用select等待连接完成
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(data_fd, &writefds);
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    int sel_ret = select(data_fd + 1, NULL, &writefds, NULL, &timeout);
    if (sel_ret <= 0)
    {
        std::cout << "Data connection timeout" << std::endl;
        close(data_fd);
        return;
    }
    // 2. 发送文件数据
    if(filename.find('\n')!=std::string::npos){
        int pos=filename.find('\n');
        filename=filename.substr(0,pos);
    }
    
    int file_fd=open(filename.c_str(),O_RDONLY);

    
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
        ssize_t sent=sendfile(data_fd, file_fd, &offset, total_size - offset);
        if (sent > 0) {
        }else if (sent == 0) {
            break;
        }else {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            } else {
                perror("sendfile error");
                break;
            }
        }
    }
    if(offset>=total_size){
        std::cout << "文件上传完毕" << std::endl;
    }
    close(file_fd);

    mark=1;
    // 3. 关闭数据连接
    shutdown(data_fd, SHUT_WR);
    close(data_fd);
    close(file_fd);
}

int fcRecv(int fd, char *buf, int len, int flags)
{
    int reallen = 0;
    fd_set set;
    struct timeval timeout;
    while (reallen < len)
    {
        int temp = recv(fd, buf + reallen, len - reallen, flags);
        if (temp < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 数据未就绪，等待可读事件
                FD_ZERO(&set);
                FD_SET(fd, &set);
                timeout.tv_sec=2;
                timeout.tv_usec=0;
                if (select(fd + 1, &set, NULL, NULL, &timeout)<=0){
                    return reallen;
                }
                continue; // 重新尝试 recv
            }
            error_report("recv", fd);
            return -1;
        }
        else if (temp == 0)
        {
            break;
        }
        reallen += temp;
    }
    return reallen;
}

int fcSend(int fd, const char *buf, int len, int flags)
{
    int reallen = 0;
    while (reallen < len){
        int temp = send(fd, buf + reallen, len - reallen, flags);
        if (temp < 0){
            error_report("send", fd);
        }
        else if (temp == 0)
        {
            // 数据已全部发送完毕
            return reallen;
        }
        reallen += temp;
    }
    return reallen;
}

void error_report(const std::string &x,int fd){
    std::cout<<x<< " start fail" << std::endl;
    close(fd);
    exit(1);
}

ftpclient::~ftpclient(){
    if (client_fd!=-1){
        shutdown(client_fd,SHUT_RDWR);
        close(client_fd);
        client_fd=-1;
    }
}