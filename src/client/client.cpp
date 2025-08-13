#include "../include/client/cli.hpp"

int main(int argc,char *argv[]){
    std::string client_ip="10.30.0.117";
    int tempport=6666;
    for(int i=0;i<argc;i++){
        if(strcmp(argv[i],"ip")==0&&i+1<argc){
            client_ip=argv[++i];
        }else if(strcmp(argv[i],"port")==0&&i+1<argc){
            tempport=std::stoi(argv[++i]);
        }
    }
    chatclient cli(client_ip,tempport);
    cli.start();
    return 0;
}