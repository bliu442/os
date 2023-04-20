#ifndef __THREAD_H__
#define __THREAD_H__

#include "../stdint.h"
#include "./mm.h"
#include "./list.h"

extern list_t thread_ready_list;
extern list_t thread_all_list;

typedef void thread_fun_t(void *func_arg);

typedef enum task_state {
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANDING,
	TASK_DIDE,
}task_state_t;

typedef struct interrupet_stack {
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	uint32_t idt_index;

	uint32_t error_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp_old;
	uint32_t ss_old;
}interrupt_stack_t;

/*
 1.用于初始化线程栈
 2.用于线程切换
 */
typedef struct thread_stack {
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;
	uint32_t eip;
	
	void *retaddr;
	thread_fun_t *function;
	void *func_arg;
}thread_stack_t;

typedef struct task {
	uint32_t stack; //r0堆栈
	uint32_t state;
	uint32_t priority; //优先级
	uint32_t ticks; //时间片
	uint32_t elapsed_ticks; //统计执行了多少时间片
	list_item_t general_list_item; //链表节点 用于存入就绪/阻塞队列
	list_item_t all_list_item; //链表节点 用于存入全部线程链表
	uint32_t cr3; //进程页表管理
	memory_pool_t user_virtual_pool; //进程虚拟地址管理
	char name[32];
	uint32_t magic; //魔数 用于判断堆栈溢出
}task_t;

typedef union task_union {
	task_t task;
	char stack[PAGE_SIZE];
}task_union_t;

extern task_t *running_thread(void);
extern void thread_create(task_union_t *pthread, thread_fun_t function, void *func_arg);
extern void thread_init(task_union_t *pthread, char *name, uint32_t priority);
extern task_t *thread_start(char *name, uint32_t priority, thread_fun_t function, void *func_arg);
extern void thread_block(task_state_t status);
extern void thread_unblock(task_t *pthread);

extern void pthread_init(void);

#endif