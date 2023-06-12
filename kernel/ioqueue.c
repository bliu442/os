#include "../include/kernel/ioqueue.h"
#include "../include/kernel/thread.h"
#include "../include/asm/system.h"

#include "../include/kernel/debug.h"

/* 队列初始化 */
void ioqueue_init(ioqueue_t *queue) {
	lock_init(&queue->lock);
	queue->producer = queue->consumer = NULL;
	queue->head = queue->tail = 0;
}

static inline int32_t next_pos(int32_t pos) {
	return (pos + 1) % buf_size;
}

/* 判断队列是否满了 */
bool ioqueue_full(ioqueue_t *queue) {
	return next_pos(queue->head) == queue->tail;
}

/* 判断队列是否空了 */
bool ioqueue_empty(ioqueue_t *queue) {
	return queue->head == queue->tail;
}

/*
 @brief 阻塞生产者/消费者
 @param waiter &queue.producer/consumer
 */
void ioqueue_wait(task_t **waiter) {
	ASSERT(*waiter == NULL && waiter != NULL);
	*waiter = running_thread();
	thread_block(TASK_BLOCKED);
}

/*
 @brief 唤醒生产者/消费者
 @param waiter &queue.producer/consumer
 */
void ioqueue_wakeup(task_t **waiter) {
	ASSERT(*waiter != NULL);
	thread_unblock(*waiter);
	*waiter = NULL;
}

/* @brief 从队列中拿出一个字符 */
char ioqueue_getchar(ioqueue_t *queue) {
	while(ioqueue_empty(queue)) {
		lock_acquire(&queue->lock);
		ioqueue_wait(&queue->consumer);
		lock_release(&queue->lock);
	}

	char byte = queue->buf[queue->tail]; //队尾取出字符
	queue->tail = next_pos(queue->tail); //队尾移动

	if(queue->producer != NULL)
		ioqueue_wakeup(&queue->producer); //唤醒生产者

	return byte;
}

/* 向队列中添加一个字符 */
void ioqueue_putchar(ioqueue_t *queue, char byte) {
	while(ioqueue_full(queue)) {
		lock_acquire(&queue->lock);
		ioqueue_wait(&queue->producer);
		lock_release(&queue->lock);
	}

	queue->buf[queue->head] = byte; //队首加入字符
	queue->head = next_pos(queue->head); //队首移动

	if(queue->consumer != NULL)
		ioqueue_wakeup(&queue->consumer); //唤醒消费者
}