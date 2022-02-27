#include"main.h"

#define SERVER_ADDR "192.168.66.128"
#define SERVER_PORT 9000

static int socket_init(){
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == sfd){
        printf("err socket\n");
        return -1;
    }
    struct sockaddr_in saddr = {0};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    if(-1 == inet_pton(AF_INET,SERVER_ADDR,&saddr.sin_addr.s_addr)){
        printf("err inet_pton\n");
        return -1;
    }
    for(int i = 0; -1 == bind(sfd,(struct sockaddr *)&saddr,sizeof(saddr)) && 20 > i; i++){
        sleep(1);
        printf("err bind,try again\n");
    }
    if(-1 == listen(sfd,10)){
        printf("err listen\n");
        return -1;
    }
    printf("ok will accept\n");
    struct sockaddr_in caddr = {0};
    socklen_t caddr_len = sizeof(caddr);
    int cfd = accept(sfd,(struct sockaddr *)&caddr,&caddr_len);
    if(-1 == cfd){
        printf("err accept\n");
        return -1;
    }
    char client_ip[INET_ADDRSTRLEN] = "";
    printf("ok accept,client ip = %s\n", inet_ntop(AF_INET,&caddr.sin_addr.s_addr,client_ip,INET_ADDRSTRLEN));
    char msg[128] = "";
    recv(cfd,msg,sizeof(char),0);
    printf("%s\n",msg);
}
int main(){
    if(-1 == socket_init()){
        return -1;
    }

    return 0;
}