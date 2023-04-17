/*
 thread process task
 thread process都是task 任务
 */

#include "../include/kernel/thread.h"
#include "../include/kernel/mm.h"
#include "../include/string.h"
#include "../include/asm/system.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/print.h"

task_union_t *main_thread;
list_t thread_ready_list;
list_t thread_all_list;

/*
 @brief 执行线程函数
 @param function 线程函数
 @param func_arg 线程函数参数
 */
static void kernel_thread(thread_fun_t *function, void *func_arg) {
	STI //执行线程函数前到确保打开中断,让时钟中断产生,实现任务切换
	function(func_arg);
}

/* @brief 获取当前运行的线程的pcb */
task_t *running_thread(void) {
	uint32_t esp = 0;
	__asm__ ("mov %0, esp;" : "=g" (esp));
	return ((task_t *)(esp & 0xFFFFF000));
}

/*
 @brief 初始化线程堆栈结构
 @param pthread 线程pcb结构体指针
 @param function 线程函数
 @param func_arg 线程函数参数

 @note 手动构建栈(结构体)
 |........| 上面用于存放interrupt_stack_t
 |func_arg|
 |function|
 |.retaddr|

 |.....eip|
 |.....esi|
 |.....edi|
 |.....ebx|
 |.....ebp| <-task.stack 初始化ABI
 */
void thread_create(task_union_t *pthread, thread_fun_t function, void *func_arg) {
	pthread->task.stack -= sizeof(interrupt_stack_t);
	pthread->task.stack -= sizeof(thread_stack_t);

	thread_stack_t *kernel_stack = (thread_stack_t *)pthread->task.stack;
	kernel_stack->ebp = kernel_stack->ebx = kernel_stack->edi = kernel_stack->esi = 0;
	kernel_stack->eip = (uint32_t)kernel_thread;
	kernel_stack->function = function;
	kernel_stack->func_arg = func_arg;
}

/*
 @brief 初始化线程pcb结构体
 @param pthread 线程pcb结构体指针
 @param name 线程名
 @param priority 线程优先级
 */
void thread_init(task_union_t *pthread, char *name, uint32_t priority) {
	if(pthread == main_thread) {
		pthread->task.state = TASK_RUNNING;
	} else {
		pthread->task.state = TASK_READY;
	}

	strcpy(pthread->task.name, name);
	pthread->task.state = TASK_RUNNING;
	pthread->task.priority = priority;
	pthread->task.ticks = priority;
	pthread->task.elapsed_ticks = 0;
	pthread->task.stack = (uint32_t)pthread + PAGE_SIZE;
	pthread->task.magic = 0x000055aa;
}

/*
 @brief 创建并执行一个线程
 @param name 线程名
 @param priority 线程优先级
 @param function 线程函数
 @param func_arg 线程函数参数
 @retval 线程pcb结构体指针
 */
task_t *thread_start(char *name, uint32_t priority, thread_fun_t function, void *func_arg) {
	task_union_t *pthread = malloc_kernel_page(1);
	memset(pthread, 0, PAGE_SIZE);

	thread_init(pthread, name, priority);
	thread_create(pthread, function, func_arg);

	ASSERT(!list_find_item(&thread_ready_list, &pthread->task.general_list_item));
	list_append(&thread_ready_list, &pthread->task.general_list_item);
	
	ASSERT(!list_find_item(&thread_all_list, &pthread->task.all_list_item));
	list_append(&thread_all_list, &pthread->task.all_list_item);

	return (task_t *)pthread;
}

/* @brief 将已经存在的执行流main更改为线程 */
static void make_main_thread(void) {
	main_thread = (task_union_t *)running_thread();
	thread_init(main_thread, "main", 31);

	ASSERT(!list_find_item(&thread_all_list, &main_thread->task.all_list_item));
	list_append(&thread_all_list, &main_thread->task.all_list_item);
}

/* @brief 初始化线程链表 添加main线程 */
void pthread_init(void) {
	put_str("pthread init \r");

	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	make_main_thread();
}
