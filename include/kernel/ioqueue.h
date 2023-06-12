#ifndef __IOQUEUE_H__
#define __IOQUEUE_H__

#include "../stdint.h"
#include "./sync.h"

#define buf_size 128

typedef struct ioqueue {
	lock_t lock;
	task_t *producer;
	task_t *consumer;
	char buf[buf_size];
	uint32_t head; //队首
	uint32_t tail; //队尾
}ioqueue_t;

extern void ioqueue_init(ioqueue_t *queue);
extern bool ioqueue_full(ioqueue_t *queue);
extern bool ioqueue_empty(ioqueue_t *queue);
extern char ioqueue_getchar(ioqueue_t *queue);
extern void ioqueue_putchar(ioqueue_t *queue, char byte);

#endif