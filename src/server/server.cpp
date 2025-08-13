#include "../include/server/ser.hpp"

int main(int argc,char *argv[]){
    std::string ip="0.0.0.0"; //无指定默认广播
    int port=6666;
    for(int i=0;i<argc;i++){
        if(strcmp(argv[i],"ip")==0&&i+1<argc){
            ip=argv[++i];
        }else if(strcmp(argv[i],"port")==0&&i+1<argc){
            port=std::stoi(argv[++i]);
        }
    }
    chatserver sever(ip,port);
    sever.start();
    return 0;
}