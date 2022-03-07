#include"main.h"

#if 0
#define SERVER_ADDR "192.168.66.128"
#else
#define SERVER_ADDR "127.0.0.1"
#endif

#define SERVER_PORT 9000
/**
 * @brief 客户端连接地址的信息
 */
// typedef struct _client_info_t{
//     int fd;
//     struct sockaddr_in client_addr;
//     socklen_t client_addr_len;
// }client_info_t;

/**
 * @brief 线程管理信息
 */
typedef struct _thread_info_t{
    node_t *node;
    list_t *list;
    pthread_t id;//线程本身id
    int pid;
    int fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
}thread_info_t;

static int get_resource();
static int socket_init();
static int socket_handle(int, list_t *);
static void *client_handle_thread(void *);
static int thread_list_delete_func(void *,void *);

int main(){
    if(-1 == get_resource()){
        return -1;
    }
    int sfd = socket_init();
    if(-1 == sfd){
        return -1;
    }
    /*建立线程链表和客户端*/
    list_t *thread_list = list_create();
    
    if(NULL == thread_list){
        return -1;
    }
    socket_handle(sfd, thread_list);

    return 0;
}
/**
 * @brief 获取系统资源情况
 * @param void
 * @return 0-OK -1-ERR
 */
static int get_resource(){
    struct rlimit resource = {0};
    if(-1 == getrlimit(RLIMIT_AS,&resource)){
        printf("getrlimit err\n");
        return -1;
    }
    printf("RLIMIT_AS=%lld %lld\n",(long long)resource.rlim_cur,(long long)resource.rlim_max);
    if(-1 == getrlimit(RLIMIT_DATA,&resource)){
        printf("getrlimit err\n");
        return -1;
    }
    printf("RLIMIT_DATA=%lld %lld\n",(long long)resource.rlim_cur,(long long)resource.rlim_max);
    if(-1 == getrlimit(RLIMIT_STACK,&resource)){
        printf("getrlimit err\n");
        return -1;
    }
    printf("RLIMIT_STACK=%lld %lld\n",(long long)resource.rlim_cur,(long long)resource.rlim_max);

    return 0;
}

/**
 * @brief 初始化socket
 * @return -1-ERR sfd-已经初始化的原套接字
 */
static int socket_init(){
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == sfd){
        printf("err socket\n");
        return -1;
    }
    struct sockaddr_in saddr = {0};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    //saddr.sin_addr.s_addr = in_addr
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
    
    return sfd;
}
/**
 * @brief 使用已初始化的套结字接收连接，每一个连接使用一个线程来处理
 * 
 * @param sfd 已初始化的套结字
 * @param thread_list 线程链表
 * @param client_list 客户端信息链表
 * 
 * @return -1:ERR
 */
static int socket_handle(int sfd, list_t *thread_list){

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    printf("main thread %ld start accept\n", pthread_self());
    while(1){
        //client_info_t *thread_info = (client_info_t *)list_insert(client_list,sizeof(client_info_t));
        thread_info_t *thread_info = (thread_info_t *)list_insert(thread_list,sizeof(thread_info_t));
        if(NULL == thread_info){
            break;
        }
        thread_info->list = thread_list;
        thread_info->client_addr_len = sizeof(thread_info->client_addr);
        thread_info->fd = accept(sfd,(struct sockaddr *)&thread_info->client_addr,&thread_info->client_addr_len);
        if(-1 == thread_info->fd){
            printf("err accept\n");
            return -1;
        }
        char client_ip[INET_ADDRSTRLEN] = "";
        printf("ok accept,client ip = %s port = %d\n", inet_ntop(AF_INET,&thread_info->client_addr.sin_addr.s_addr,client_ip,INET_ADDRSTRLEN), thread_info->client_addr.sin_port);
        if(0 == pthread_create(&thread_info->id,&thread_attr,client_handle_thread,thread_info)){
            printf("thread %ld create ok\nsocket fd %d\n", thread_info->id, thread_info->fd);
        }else{
            //if(0 == list_delete(thread_list, thread_info)){
                printf("thread create err,list_delete ok\n");
            //}
        }
    }
}
/**
 * @brief 客户端处理线程，每个连接一个线程来处理
 */
static void *client_handle_thread(void *arg){
    //sleep(3);
    printf("thread %ld start\n", pthread_self());
    thread_info_t *thread_info = (thread_info_t *)arg;
    struct stat file_info = {0};
    int file_p = 0;
    char msg[4096] = {0};
    // FILE *fp = fopen("./main.html","r");
    // if(NULL == fp){
    //     printf("open file err\n");
    // }
    if(-1 == stat("./main.html",&file_info)){
        printf("stat file err\n");
    }
    if(-1 == (file_p = open("main.html",O_RDONLY))){
        printf("open file err\n");
    }
    int i = 0;
    i = recv(thread_info->fd,msg,sizeof(msg),0);
    printf("hello world\n%s",msg);

    memset(msg,0,sizeof(msg));
    sprintf(msg,"HTTP/1.1 200 OK\nConnection: close\nContent-length: %ld\nContent-type: text/html\n\n",file_info.st_size);
    if(-1 == send(thread_info->fd,msg,strlen(msg),0)){
        printf("send err\n");
    }
    off_t start = SEEK_SET;
    ssize_t size = sendfile(thread_info->fd,file_p,&start,(size_t)file_info.st_size);
    printf("html send ok\n");
    if(0 == list_delete(thread_info->list,thread_info)){
        printf("delete ok\n");
    }else{
        printf("delete err\n");
    }
    // while(1){
    //     i = recv(thread_info->fd,msg,sizeof(msg),0);
    //     //-1:接收错误 0:连接断开
    //     if(-1 == i){
    //         printf("recv err -1\n");
    //         break;
    //     }else if(0 == i){
    //         //if(1 == list_delete(thread_info->list,thread_list_delete_func,(void *)thread_info->id)){
    //         if(0 == list_delete(thread_info->list,thread_info)){
    //             printf("delete ok\n");
    //         }else{
    //             printf("delete err\n");
    //         }
    //         break;
    //     }else{
    //         printf("hello world\n%s",msg);

    //         memset(msg,0,sizeof(msg));
    //         sprintf(msg,"HTTP/1.1 200 OK\nConnection: close\nContent-length: %ld\nContent-type: text/html\n\n",file_info.st_size);
    //         if(-1 == send(thread_info->fd,msg,strlen(msg),0)){
    //             printf("send err\n");
    //             continue;
    //         }

    //         // printf("send over\n");
    //         off_t start = SEEK_SET;
    //         ssize_t size = sendfile(thread_info->fd,file_p,&start,(size_t)file_info.st_size);
    //         //printf("start = %ld size = %ld file_info.st_size = %ld\n",start,size,file_info.st_size);    
    //     }
        
    // }
    printf("thread:%ld will exit\n",pthread_self());
    pthread_exit(NULL);
}
static int thread_list_delete_func(void *node,void *arg){
    thread_info_t *thread = (thread_info_t *)node;
    int *id = (int *)arg;
    if(thread->id == *id){
        return 1;
    }
    return 0;
}