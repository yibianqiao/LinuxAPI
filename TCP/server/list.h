/**
 * @brief 通用链表，线程安全
 */
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>

typedef struct _list_t list_t;
typedef struct _node_t node_t;

extern list_t *list_create();
extern void *list_insert(list_t *,size_t);
extern int list_find_num(list_t *);
extern int list_delete(list_t *list, void *);
//extern int list_delete(list_t *,int (*func)(void *,void *),void *);