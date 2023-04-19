/*
 多个任务共享一套公共资源 临界区
 互斥 竞争条件 原子性 信号量 P/V操作
 二元信号量实现互斥锁
 */

#include "../include/kernel/sync.h"
#include "../include/asm/system.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/thread.h"

/*
 @brief 信号量初始化
 @param semaphore 信号量结构体指针
 @param value 信号量初始值
 */
void semaphore_init(semaphore_t *semaphore, uint8_t value) {
	semaphore->value = value;
	list_init(&semaphore->waiters);
}

/*
 @brief 申请一个信号量
 @param semaphore 信号量结构体指针
 */
void semaphore_down(semaphore_t *semaphore) {
	CLI_FUNC

	while(semaphore->value == 0) {
		ASSERT(!list_find_item(&semaphore->waiters, &running_thread()->general_list_item));
		list_append(&semaphore->waiters, &running_thread()->general_list_item); //加入该信号量的等待队列
		thread_block(TASK_BLOCKED);
	}

	semaphore->value--;

	STI_FUNC
}

/*
 @brief 释放一个信号量
 @param semaphore 信号量结构体指针
 */
void semaphore_up(semaphore_t *semaphore) {
	CLI_FUNC
	
	if(!list_empty(&semaphore->waiters)) {
		task_t *blocked = item2entry(task_t, general_list_item, list_pop(&semaphore->waiters)); //从该信号量的等待队列中弹出	
		thread_unblock(blocked);
	}

	semaphore->value++;

	STI_FUNC
}

/*
 @brief 锁初始化
 @param lock 锁结构体指针
 */
void lock_init(lock_t *lock) {
	lock->holder = NULL;
	lock->holder_repeat_nr = 0;
	semaphore_init(&lock->seamphore, 1);
}

/*
 @brief 申请锁
 @param lock 锁结构体指针
 */
void lock_acquire(lock_t *lock) {
	if(lock->holder != running_thread()) {
		semaphore_down(&lock->seamphore);
		lock->holder = running_thread();
		lock->holder_repeat_nr = 1;
	} else {
		lock->holder_repeat_nr++;
	}
}

/*
 @brief 释放锁
 @param lock 锁结构体指针
 */
void lock_release(lock_t *lock) {
	ASSERT(lock->holder == running_thread());
	if(lock->holder_repeat_nr > 1) {
		lock->holder_repeat_nr--;
		return;
	}

	lock->holder = NULL;
	lock->holder_repeat_nr = 0;
	semaphore_up(&lock->seamphore);
}