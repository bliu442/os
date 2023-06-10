/* 双向链表 主要是为了链 */

#include "../include/kernel/list.h"
#include "../include/asm/system.h"

/*
 @brief 初始化双向链表
 @param list 链表指针
 */
void list_init(list_t *list) {
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

/*
 @brief 向双向链表中插入一个节点
 @param before 插入到before节点前
 @param item 要插入的节点
 */
void list_insert_before(list_item_t *before, list_item_t *item) {
	CLI_FUNC

	before->prev->next = item;

	item->prev = before->prev;
	item->next = before;

	before->prev = item;

	STI_FUNC
}

/*
 @brief 向链表压入一个节点
 @param list 链表指针
 @param item 要压入的节点
 */
void list_push(list_t *list, list_item_t *item) {
	list_insert_before(list->head.next, item);
}

/*
 @brief 向链表追加一个节点
 @param list 链表指针
 @param item 要追加的节点
 */
void list_append(list_t *list, list_item_t *item) {
	list_insert_before(&list->tail, item);
}

/*
 @brief 从链表移除一个节点
 @param item 要移除的节点
 */
void list_remove(list_item_t *item) {
	CLI_FUNC

	item->prev->next = item->next;
	item->next->prev = item->prev;

	STI_FUNC
}

/*
 @brief 从链表弹出一个节点
 @param list 链表指针
 @retval 弹出的节点
 */
list_item_t *list_pop(list_t *list) {
	list_item_t *item = list->head.next;
	
	list_remove(item);
	
	return item;
}

/*
 @brief 判断一个节点是否存在与链表中
 @param list 链表指针
 @param item 待查找的节点
 @retval true 该节点在链表中
 false 该节点不在链表中
 */
bool list_find_item(list_t *list, list_item_t *item) {
	list_item_t *ptr = list->head.next;
	while(ptr != &list->tail) {
		if(ptr == item)
			return true;
		else
			ptr = ptr->next;
	}

	return false;
}

/*
 @brief 查看链表有多少节点
 @param list 链表指针
 @retval 链表节点长度
 */
uint32_t list_len(list_t *list) {
	list_item_t *ptr = list->head.next;
	uint32_t len = 0;
	while(ptr != &list->tail) {
		++len;
		ptr = ptr->next;
	}

	return len;
}

/*
 @brief 判断链表是否为空
 @retval true 链表为空
 false 链表不空
 */
bool list_empty(list_t *list) {
	return (list->head.next == &list->tail ? true : false);
}

/*
 @brief 按照固定的格式查找链表中是否有符合条件的节点
 @param list 链表指针
 @param func 条件判断回调函数
 @param arg 判断条件
 @retval 找到的节点/NULL
 */
list_item_t *list_traverasl(list_t *list, function func, int arg) {
	list_item_t *item = list->head.next;
	
	if(list_empty(list))
		return NULL;
	
	while(item != &list->tail) {
		if(func(item, arg))
			return item;

		item = item->next;
	}

	return NULL;
}



