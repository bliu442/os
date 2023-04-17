#ifndef __LIST_H__
#define __LIST_H__

#include "../stdint.h"

#define list_offset(struct_type, member) (int)(&((struct_type *)0)->member) //结构体成员member在结构体中的偏移量
#define item2entry(struct_type, struct_member_name, item_ptr) (struct_type *)((int)item_ptr - list_offset(struct_type, struct_member_name)) //根据结构体成员地址找到结构体起始地址

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