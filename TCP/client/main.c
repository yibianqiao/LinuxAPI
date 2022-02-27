#include"main.h"

#define CLIENT_IP "192.168.66.128"
#define CLIENT_PORT 9999
#define SERVER_IP "192.168.66.128"
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
    struct sockaddr_in saddr = {0};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    if(-1 == inet_pton(AF_INET,SERVER_IP,&saddr.sin_addr.s_addr)){
        printf("err inet_pton\n");
        return -1;
    }
    if(-1 == connect(fd,(struct sockaddr *)&saddr,sizeof(saddr))){
        printf("err connect\n");
        return -1;
    }
    char msg[] = "hello world\n";
    send(fd,msg,sizeof(char),0);
    // if(-1 == listen(fd,10)){
    //     return -1;
    // }
    // struct sockaddr_in caddr = {0};
    // int cfd = accept(fd,(struct sockaddr *)&caddr,sizeof(caddr));
    // if(-1 == cfd){
    //     return -1;
    // }
    // char client_ip[INET_ADDRSTRLEN] = "";
    // printf("ok accept,client ip = %d", inet_ntop(AF_INET,caddr.sin_addr.s_addr,client_ip,INET_ADDRSTRLEN));
}
int main(){
    if(-1 == socket_init()){
        return -1;
    }

    return 0;
}