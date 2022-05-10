
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<unistd.h>
#include<time.h>
#include<string.h>
#include<errno.h>

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/resource.h>
#include<sys/stat.h>
#include<sys/sendfile.h>
#include<fcntl.h>

#include"list.h"

#define FILE_PATH "./001.bmp"

#if 1
#define SERVER_ADDR "192.168.0.103"
#else
#define SERVER_ADDR "127.0.0.1"
#endif

#define SERVER_PORT 9000


/**
 * @brief 线程管理信息
 */
typedef struct thread_info_t{
    void *node;//所属节点
    list_t *list;//所属链表
    pthread_t id;//线程id
    int fd;//socket套接字
    struct sockaddr_in client_addr;//客户端地址信息
    socklen_t client_addr_len;//客户端地址长度
    time_t time_create;//连接、线程创建时间
}thread_info_t;


#pragma pack(push, 1)
/**
 * @brief bmp文件头信息数据结构
 */
typedef struct bmp_info_t{
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
    //
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImages;
    uint32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    int32_t biClrUsed;
    uint32_t biClrImportant;
}bmp_info_t;
#pragma pack(pop)

static int get_resource();
static int socket_init();
static int socket_handle(int, list_t *);
static void *client_handle_thread(void *);
static void thread_clean_func(void *);
// static int thread_list_delete_func(void *,void *);
/**
 * @author liteng
 * @date 2022/3/7
 * @brief ESP8266TCP服务器，给ESP8266发送图片
 */
int main(int argc, void *argv[]){
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
    if(0 != socket_handle(sfd, thread_list)){
        return -1;
    }
    //退出前清理链表，终止所有子线程
    int thread_num = list_find_num(thread_list);
    if(0 == thread_num){
        if(-1 == list_free(thread_list)){
            return -1;
        }
    }else{
        for(int i = 1; i <= thread_num; i++){
            thread_info_t *thread_info = (thread_info_t *)list_find_data(thread_list, i);
            if(NULL != thread_info){
                pthread_cancel(thread_info->id);
                // pthread_join(thread_info->id, NULL);
            }
        }
    }
    
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
        DEBUG("getrlimit err\n");
        return -1;
    }
    DEBUG("RLIMIT_AS=%lld %lld\n",(long long)resource.rlim_cur,(long long)resource.rlim_max);
    if(-1 == getrlimit(RLIMIT_DATA,&resource)){
        DEBUG("getrlimit err\n");
        return -1;
    }
    DEBUG("RLIMIT_DATA=%lld %lld\n",(long long)resource.rlim_cur,(long long)resource.rlim_max);
    if(-1 == getrlimit(RLIMIT_STACK,&resource)){
        DEBUG("getrlimit err\n");
        return -1;
    }
    DEBUG("RLIMIT_STACK=%lld %lld\n",(long long)resource.rlim_cur,(long long)resource.rlim_max);

    return 0;
}

/**
 * @brief 初始化socket
 * @return -1-ERR sfd-已经初始化的原套接字
 */
static int socket_init(){
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == sfd){
        DEBUG("err socket\n");
        return -1;
    }
    int optval = 1;
    if(-1 == setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))){
        DEBUG("setsockopt err\n");
    }
    struct sockaddr_in saddr = {0};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    //saddr.sin_addr.s_addr = in_addr
    if(-1 == inet_pton(AF_INET, SERVER_ADDR, &saddr.sin_addr)){
        DEBUG("err inet_pton\n");
        return -1;
    }
    for(int i = 0; -1 == bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) && 20 > i; i++){
        sleep(1);
        DEBUG("err bind,try again\n");
    }
    if(-1 == listen(sfd,10)){
        DEBUG("err listen\n");
        return -1;
    }
    DEBUG("socket init is ok\n");
    char client_ip[INET_ADDRSTRLEN] = "";
    //打印客户端地址
    if(NULL == inet_ntop(AF_INET, &saddr.sin_addr, client_ip, INET_ADDRSTRLEN)){
        DEBUG("inet_ntop err\n");
    }
    DEBUG("main ip = %s port = %d\n", client_ip, saddr.sin_port);

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
    //初始化线程属性
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    DEBUG("main thread:0x%lx start accept connect\n", pthread_self());

    char client_ip[INET_ADDRSTRLEN] = "";
    thread_info_t *thread_info = NULL;
    int optval = 1;

    //循环接收连接
    while(1){
        //使用链表保存每个连接
        thread_info = (thread_info_t *)list_insert(thread_list, sizeof(thread_info_t));
        if(NULL == thread_info){
            break;
        }
        thread_info->list = thread_list;
        thread_info->client_addr_len = sizeof(thread_info->client_addr);
        thread_info->fd = accept(sfd, (struct sockaddr *)&thread_info->client_addr, &thread_info->client_addr_len);
        if(-1 == thread_info->fd){
            DEBUG("err accept\n");
            if(0 == list_delete(thread_list, thread_info)){
                DEBUG("main:thread create err,list_delete ok\n");
            }
            continue;
        }
        //设置socket属性
        if(-1 == setsockopt(thread_info->fd, SOL_SOCKET,SO_REUSEADDR, &optval, sizeof(optval))){
            DEBUG("setsockopt err\n");
        }
        thread_info->time_create = time(NULL);
        //打印客户端地址
        if(NULL == inet_ntop(AF_INET, &thread_info->client_addr.sin_addr, client_ip, INET_ADDRSTRLEN)){
            DEBUG("inet_ntop err\n");
        }
        DEBUG("accept ok client ip = %s port = %d\n", client_ip, thread_info->client_addr.sin_port);
        //启动线程处理连接
        if(0 == pthread_create(&thread_info->id, &thread_attr, client_handle_thread, thread_info)){
            //DEBUG("main:thread %ld create ok\nsocket fd %d\n", thread_info->id, thread_info->fd);
        }else{
            if(0 == list_delete(thread_list, thread_info)){
                DEBUG("main:thread create err,list_delete ok\n");
            }
        }
    }
    //销毁线程属性
    if(0 != pthread_attr_destroy(&thread_attr)){
        DEBUG("pthread_attr_destroy err\n");
    }
    return 0;
}
/**
 * @brief 客户端处理线程，每个连接一个线程来处理
 * 线程退出前删除自己的链表节点，为防止线程异常结束未删除，使用线程退出处理函数
 */
#if 0
static void *client_handle_thread(void *arg){
    #define BUF_NUM 14400
    thread_info_t *thread_info = (thread_info_t *)arg;
    // struct stat file_info = {0};
    //发送缓冲区
    uint16_t buf[BUF_NUM] = { 0 };
    //从图片中读取数据缓冲区
    uint32_t buffer[BUF_NUM] = { 0 };
    bmp_info_t bmp_info = { 0 };

    DEBUG("thread:%lx start\n", thread_info->id);
    //注册线程；退出处理函数
    pthread_cleanup_push(thread_clean_func, arg);

    int path_arg = 0;
    FILE *fp = NULL;
    while(1){
        if(0 == path_arg){
            fp = fopen(FILE_PATH,"r");
            path_arg = 1;
        }else{
            fp = fopen("./002.bmp","r");
            path_arg = 0;
        }
        // fp = fopen("./002.bmp","r");

        if(NULL == fp){
            DEBUG("open file err\n");
        }
        //读取bmp文件头信息，偏移至数据位置
        fread(&bmp_info, sizeof(bmp_info_t), 1, fp);
        fseek(fp, bmp_info.bfOffBits, SEEK_SET);
        //写入ILI9341
        int len = 0;
        while(1){
            memset(buffer, 0, sizeof(buffer));
            memset(buf, 0, sizeof(buf));
            //根据实际图片像素保存格式读取，这里是一个像素点使用32位表示，这样直接读取大于1字节会涉及到大小端的问题，即读取的数据怎么放入这32位中
            //这里RGB32格式空白的字节保存在最高位，并且全为1,即FF
            len = fread(buffer, sizeof(uint32_t), BUF_NUM, fp);
            if(0 >= len){
                DEBUG("read over len = %d\n", len);
                break;
            }
            // printf("len = %d\n", len);
            uint16_t middle = 0;
            for(int i = 0; i < len; i++){
                buf[i] = 0;
                buf[i] |= (buffer[i] & 0x000000f8) >> 3;
                buf[i] |= (buffer[i] & 0x0000fc00) >> 5;
                buf[i] |= (buffer[i] & 0x00f80000) >> 8;
                middle = buf[i] & 0x00ff;
                buf[i] = buf[i] >> 8;
                buf[i] = (middle << 8) | buf[i];
            }
            for(int i = 0; i < len; ){
                i += send(thread_info->fd, buf, sizeof(uint16_t) * (len - i), 0);
            }
            // if(-1 == send(thread_info->fd, buf, sizeof(uint16_t) * len, MSG_WAITALL)){
            //     break;
            // }
            // sleep(1);
        }
        fclose(fp);
        // sleep(1);
    }


    
    DEBUG("thread:%lx will exit\n", thread_info->id);
    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}
#else
static void *client_handle_thread(void *arg){
    #define BUF_NUM 9600
    thread_info_t *thread_info = (thread_info_t *)arg;
    // struct stat file_info = {0};
    //发送缓冲区
    uint8_t buf[BUF_NUM] = { 0 };
    //从图片中读取数据缓冲区
    uint8_t buffer[2 * BUF_NUM] = { 0 };
    bmp_info_t bmp_info = { 0 };

    DEBUG("thread:%lx start\n", thread_info->id);
    //注册线程；退出处理函数
    pthread_cleanup_push(thread_clean_func, arg);

    int path_arg = 0;
    FILE *fp = NULL;
    while(1){
        if(0 == path_arg){
            fp = fopen(FILE_PATH,"r");
            path_arg = 1;
        }else{
            fp = fopen("./002.bmp","r");
            path_arg = 0;
        }
        
        if(NULL == fp){
            DEBUG("open file err\n");
        }
        //读取bmp文件头信息，偏移至数据位置
        fread(&bmp_info, sizeof(bmp_info_t), 1, fp);
        fseek(fp, bmp_info.bfOffBits, SEEK_SET);
        //写入ILI9341
        int len = 0;
        while(1){
            memset(buffer, 0, sizeof(buffer));
            memset(buf, 0, sizeof(buf));
            //根据实际图片像素保存格式读取，这里是一个像素点使用32位表示，这样直接读取大于1字节会涉及到大小端的问题，即读取的数据怎么放入这32位中
            //按照字节读取，第4个字节是FF，读取顺序是BGR
            len = fread(buffer, sizeof(uint8_t), sizeof(buffer), fp);
            if(0 >= len){
                DEBUG("read over len = %d\n", len);
                break;
            }
            //将RGB32格式换为RGB565，数组按照RGB的顺序放，发送过去是正确的
            for(int i = 0; i < BUF_NUM/2; i++){
                buf[i * 2] |= (buffer[i * 4 + 2] & 0xf8);
                buf[i * 2] |= (buffer[i * 4 + 1] & 0xe0) >> 5;
                buf[i * 2 + 1] |= (buffer[i * 4 + 1] & 0x1c) << 3;
                buf[i * 2 + 1] |= (buffer[i * 4] & 0xf8) >> 3;
            }
            // printf("len = %d\n", len);
            if(-1 == send(thread_info->fd, buf, sizeof(buf), MSG_WAITALL)){
                break;
            }
            // sleep(1);
        }
        fclose(fp);
        // sleep(1);
    }


    
    DEBUG("thread:%lx will exit\n", thread_info->id);
    pthread_cleanup_pop(1);
    pthread_exit(NULL);
}
#endif
/**
 * @brief 线程退出清理函数
 */
static void thread_clean_func(void *arg){
    thread_info_t *thread_info = (thread_info_t *)arg;
    if(0 == list_delete(thread_info->list, thread_info)){
        DEBUG("thread:%ld delete ok\n", thread_info->id);
    }else{
        DEBUG("thread:%ld delete err\n", thread_info->id);
    }
}