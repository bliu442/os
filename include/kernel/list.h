#ifndef __LIST_H__
#define __LIST_H__

#include "../stdint.h"

typedef struct list_item {
	struct list_item *prev; //前一个
	struct list_item *next; //后一个
}list_item_t;

typedef struct list {
	list_item_t head; //链表头
	list_item_t tail; //链表尾
}list_t;

typedef bool (function)(list_item_t *, int);

extern void list_init(list_t *list);
extern void list_push(list_t *list, list_item_t *item);
extern list_item_t *list_pop(list_t *list);

extern void list_insert_before(list_item_t *before, list_item_t *item);
extern void list_remove(list_item_t *item);

extern void list_append(list_t *list, list_item_t *item);

extern uint32_t list_len(list_t *list);
extern bool list_empty(list_t *list);

extern bool list_find_item(list_t *list, list_item_t *item);
extern list_item_t *list_traverasl(list_t *list, function func, int arg);

#endif