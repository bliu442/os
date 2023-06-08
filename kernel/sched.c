/* 任务调度算法 */

#include "../include/kernel/thread.h"
#include "../include/asm/system.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/process.h"

extern void switch_to(task_t *current, task_t *next); //see sched.asm

static list_item_t *thread_list_next_item;
extern task_union_t *idle_thread;

/* @brief 任务调度函数 */
void schedule(void) {
	CLI

	task_t *current = running_thread();
	if(current->state == TASK_RUNNING) { //时间片用完的执行流直接进入就绪链表
		ASSERT(!list_find_item(&thread_ready_list, &current->general_list_item));
		list_append(&thread_ready_list, &current->general_list_item);

		current->ticks = current->priority;
		current->state = TASK_READY;
	} else {
		//阻塞等其他状态
	}

	if(list_empty(&thread_ready_list)) {
		thread_unblock((task_t *)idle_thread);
	}
	thread_list_next_item = list_pop(&thread_ready_list);
	
	task_t *next = item2entry(task_t, general_list_item, thread_list_next_item);
	next->state = TASK_RUNNING;

	/* 更改cr3寄存器的值,实现虚拟地址的隔离 */
	process_activate(next);

	/*
	 @note 调用汇编函数switch_to
	 @note 堆栈图
	 |........|
	 |....next|
	 |.current|
	 |.retaddr| <-esp 进入汇编函数switch_to
	 */
	switch_to(current, next);
}