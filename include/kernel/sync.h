#ifndef __SYNC_H__
#define __SYNC_H__

#include "./list.h"
#include "../stdint.h"

typedef struct task task_t;

/* 信号量 */
typedef struct semaphore {
	uint8_t value;
	list_t waiters; //信号量等待队列
}semaphore_t;

/* 锁 */
typedef struct lock {
	semaphore_t seamphore; //二元信号量实现锁
	task_t *holder; //锁的持有者
	uint32_t holder_repeat_nr; //锁的持有者重复申请锁的次数
}lock_t;

extern void semaphore_init(semaphore_t *semaphore, uint8_t value);
extern void semaphore_down(semaphore_t *semaphore);
extern void semaphore_up(semaphore_t *semaphore);

extern void lock_init(lock_t *lock);
extern void lock_acquire(lock_t *lock);
extern void lock_release(lock_t *lock);

#endif