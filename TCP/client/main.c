#include<string.h>
#include<errno.h>

#include"main.h"

#define DEBUG_
#ifdef DEBUG_
#define DEBUG(format, ...) printf("%s:%d:errno=%d\t"format, __func__, __LINE__, errno, ##__VA_ARGS__)
#endif

#define CLIENT_IP "192.168.66.128"
#define CLIENT_PORT 9999
#if 1
#define SERVER_IP "192.168.66.128"
#else
#define SERVER_IP "127.0.0.1"
#endif
#define SERVER_PORT 9000

static int socket_init(){
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == fd){
        printf("err socket\n");
        return -1;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CLIENT_PORT);
    if(-1 == inet_pton(AF_INET,CLIENT_IP,&addr.sin_addr.s_addr)){
        printf("err inet_pton\n");
        return -1;
    }
    for(int i = 0; -1 == bind(fd,(struct sockaddr *)&addr,sizeof(addr)) && 20 > i; i++){
        sleep(1);
        printf("err bind,try again\n");
    }
    char client_ip[INET_ADDRSTRLEN] = "";
    //打印客户端地址
    if(NULL == inet_ntop(AF_INET, &addr.sin_addr, client_ip, INET_ADDRSTRLEN)){
        DEBUG("inet_ntop err\n");
    }
    DEBUG("client ip = %s port = %d\n", client_ip, addr.sin_port);

    struct sockaddr_in saddr = {0};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    if(-1 == inet_pton(AF_INET,SERVER_IP,&saddr.sin_addr.s_addr)){
        printf("err inet_pton\n");
        return -1;
    }
    if(NULL == inet_ntop(AF_INET, &saddr.sin_addr, client_ip, INET_ADDRSTRLEN)){
        DEBUG("inet_ntop err\n");
    }
    DEBUG("server ip = %s port = %d\n", client_ip, saddr.sin_port);

    while(-1 == connect(fd,(struct sockaddr *)&saddr,sizeof(saddr))){
        printf("err connect\n");
        sleep(2);
    }
    char msg[] = "hello world\n";
    send(fd,msg,sizeof(msg),0);
    while(1){
        memset(msg, 0, sizeof(msg));
        int len = recv(fd, msg, sizeof(msg), 0);
        printf("len = %d\n", len);
        if(0 >= len){
            break;
        }
        printf("recv %s", msg);
    }
}
int main(){
    if(-1 == socket_init()){
        return -1;
    }

    return 0;
}