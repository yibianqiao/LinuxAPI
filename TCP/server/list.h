/**
 * @brief 通用链表，线程安全，用户自定义的data结构，必须使用void *类型作为第一个属性，保存其所在的节点
 * @brief 这样做的目的是释放节点更快，无需在链表中查询节点，代价就是第一个成员必须是void *
 */
#include<stdlib.h>

#define DEBUG_
#ifdef DEBUG_
#define DEBUG(format, ...) printf("%s:%d:errno=%d\t"format, __func__, __LINE__, errno, ##__VA_ARGS__)
#endif

typedef struct _list_t list_t;

extern list_t *list_create();
extern void *list_insert(list_t *, size_t);
extern int list_find_num(list_t *);
extern void *list_find_data(list_t *, int);
extern int list_delete(list_t *, void *);
extern int list_free(list_t *);
//extern int list_delete(list_t *,int (*func)(void *,void *),void *);