#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<errno.h>

#include"list.h"

typedef struct _node_t{
    void *data;
    struct _node_t *front;
    struct _node_t *last;
    //void *data;
}node_t;
/**
 * @brief 链表属性
 */
struct _list_t{
    int num;
    node_t *head;
    pthread_mutex_t mutex;
    //pthread_mutexattr_t mutexattr;
};
/**
 * @brief 申请并初始化一个链表
 */
extern list_t *list_create(){
    list_t *p = (list_t *)calloc(1, sizeof(list_t));
    if(NULL == p) return NULL;
    memset(p, 0, sizeof(list_t));
    p->head = NULL;
    p->num = 0;
    // p->mutex = PTHREAD_MUTEX_INITIALIZER;
    // pthread_mutexattr_init(&p->mutexattr);
    if(0 != pthread_mutex_init(&p->mutex,NULL)){
        DEBUG("list mutex init err\n");
        free(p);
        p = NULL;
    }
    return p;
}
/**
 * @brief 向链表中插入一个节点
 * @param size：保存的数据大小
 * @return 申请空间的指针
 */
extern void *list_insert(list_t *list,size_t size){
    if(NULL == list || 0 >= size){
        return NULL;
    }
    node_t *new = (node_t *)calloc(1, sizeof(node_t));
    if(NULL == new) return NULL;
    new->data = calloc(size, 1);
    if(NULL == new->data){
        free(new);
        return NULL;
    }
    //data结构体的第一个字段需要是node_t *类型，指向自己的节点
    *(node_t **)(new->data) = new;
    new->last = NULL;

    if(0 != pthread_mutex_lock(&list->mutex)){
        DEBUG("list mutex lock err\n");
    }else{
        DEBUG("list mutex lock ok\n");
    }
    //空链表
    if(NULL == list->head){
        new->front = NULL;
        list->head = new;
        list->num++;
        if(0 != pthread_mutex_unlock(&list->mutex)){
            DEBUG("list mutex unlock err\n");
        }else{
            DEBUG("list mutex unlock ok\n");
        }
        return new->data;
    }

    node_t *node = list->head;
    while(NULL != node->last){
        node = node->last;
    }
    new->front = node;
    node->last = new;
    list->num++;

    if(0 != pthread_mutex_unlock(&list->mutex)){
        DEBUG("list mutex unlock err\n");
    }else{
        DEBUG("list mutex unlock ok\n");
    }
    DEBUG("list_num = %d\n", list->num);
    return new->data;
}
/**
 * @brief 获取链表节点数量
 * @return -1:err
 */
extern int list_find_num(list_t *list){
    if(NULL == list){
        DEBUG("list_delete arg err\n");
        return -1;
    }
    if(0 != pthread_mutex_lock(&list->mutex)){
        DEBUG("list mutex lock err\n");
    }
    return list->num;
}
/**
 * @brief 查找指定节点数据
 */
extern void *list_find_data(list_t *list, int num){
    if(num > list_find_num(list)){
        DEBUG("num is out\n");
        return NULL;
    }
    node_t *node = list->head;
    for(int i = 1; i < num; i++){
        if(NULL == node->last){
            return NULL;
        }
        node = node->last;
    }
    return node->data;
}
/**
 * @brief 用户指定删除条件在链表中进行删除
 * @param list:链表
 * @param func:自定义删除条件
 * @return 0:删除成功
 */
extern int list_delete(list_t *list, void *data){
    if(NULL == list || NULL == data){
        DEBUG("list_delete arg err\n");
        return -1;
    }
    if(0 != pthread_mutex_lock(&list->mutex)){
        DEBUG("list mutex lock err\n");
    }
    //取出data保存的自己所在的node指针，用于后续释放
    node_t *node = *(node_t **)data;

    if(NULL != node->front && NULL != node->last){
        node->front->last = node->last;
        node->last->front = node->front;
    }else if(NULL == node->front){
        list->head = node->last;
        node->last->front = NULL;
    }else{
        node->front->last = node->last;
    }
    
    list->num--;
    free(data);
    free(node);
    data = NULL;
    node = NULL;
    if(0 != pthread_mutex_unlock(&list->mutex)){
        DEBUG("list mutex unlock err\n");
    }
    DEBUG("list_num = %d\n", list->num);
    return 0;
}
/**
 * @brief 释放整个链表
 * @return -1:err 销毁的节点数量
 */
extern int list_free(list_t *list){
    if(NULL == list){
        DEBUG("err\n");
        return -1;
    }
    if(0 != pthread_mutex_lock(&list->mutex)){
        DEBUG("list mutex lock err\n");
    }
    if(NULL == list->head){
        DEBUG("num = %d\n", list->num);
        list->num = 0;
        return 0;
    }
    node_t *node = NULL;
    int num = 0;
    while(NULL != list->head){
        node = list->head;
        list->head = list->head->last;
        free(node);
        num++;
    }
    return num;
}