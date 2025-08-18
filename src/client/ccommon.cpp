#include "../include/ccommon.hpp"

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
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                break;
            }else{
                perror("send");
                close(fd);
                return reallen;
            }
        }
        else if (temp == 0){
            // 数据已全部发送完毕
            return reallen;
        }
        reallen += temp;
    }
    return reallen;
}