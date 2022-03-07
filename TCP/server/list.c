#include"list.h"

struct _node_t{
    void *data;
    struct _node_t *front;
    struct _node_t *last;
    //void *data;
};

struct _list_t{
    int num;
    node_t *head;
    pthread_mutex_t mutex;
    //pthread_mutexattr_t mutexattr;
};

extern list_t *list_create(){
    list_t *p = (list_t *)calloc(1, sizeof(list_t));
    if(NULL == p) return NULL;
    memset(p, 0, sizeof(list_t));
    p->head = NULL;
    p->num = 0;
    // p->mutex = PTHREAD_MUTEX_INITIALIZER;
    // pthread_mutexattr_init(&p->mutexattr);
    if(0 != pthread_mutex_init(&p->mutex,NULL)){
        printf("list mutex init err\n");
        free(p);
        p = NULL;
    }
    return p;
}
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
        printf("list mutex lock err\n");
    }else{
        printf("list mutex lock ok\n");
    }
    //空链表
    if(NULL == list->head){
        new->front = NULL;
        list->head = new;
        list->num++;
        if(0 != pthread_mutex_unlock(&list->mutex)){
            printf("list mutex unlock err\n");
        }else{
            printf("list mutex unlock ok\n");
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
        printf("list mutex unlock err\n");
    }else{
        printf("list mutex unlock ok\n");
    }
    return new->data;
}
extern int list_find_num(list_t *list){

}
/**
 * @brief 用户指定删除条件在链表中进行删除
 * @param list:链表
 * @param func:自定义删除条件
 * @return 0:删除成功
 */
extern int list_delete(list_t *list, void *data){
    if(NULL == list || NULL == data){
        printf("list_delete arg err\n");
        return -1;
    }
    if(0 != pthread_mutex_lock(&list->mutex)){
        printf("list mutex lock err\n");
    }
    node_t *node = (node_t *)data;
    //node_t *node = (node_t *)node;
    // if(NULL == node){
    //     return -1;
    // }
    if(NULL != node->front){
        node->front->last = node->last;
    }
    if(NULL != node->last){
        node->last->front = node->front;
    }
    
    list->num--;
    free(data);
    free(node);
    data = NULL;
    node = NULL;
    if(0 != pthread_mutex_unlock(&list->mutex)){
        printf("list mutex unlock err\n");
    }
    return 0;
}
// extern int list_delete(list_t *list,int (*func)(void *,void *),void *arg){
//     pthread_mutex_lock(&list->mutex);
//     node_t *node = list->head;
//     while(NULL != node){
//         if(1 == func(node->data,arg)){
//             node->front->last = node->last;
//             node->last->front = node->front;
//             list->num--;
//             free(node->data);
//             free(node);
//             node->data = NULL;
//             node = NULL;
//             return 1;
//         }
//         node = node->last;
//     }
//     pthread_mutex_unlock(&list->mutex);
//     return 0;
// }