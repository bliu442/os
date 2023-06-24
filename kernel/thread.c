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
#include "../include/kernel/sched.h"
#include "../include/unistd.h"
#include "../include/kernel/fs.h"

task_union_t *main_thread;
task_union_t *idle_thread;
list_t thread_ready_list;
list_t thread_all_list;
lock_t pid_lock;

static void idle(void *arg) {
	while(1) {
		thread_block(TASK_BLOCKED);
		__asm__ volatile("sti; hlt");
	}
}

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

/* 获取当前任务pid */
pid_t thread_get_pid(void) {
	return running_thread()->pid;
}

/* brief 申请pid */
static pid_t thread_allocate_pid(void) {
	static pid_t pid = 0;
	lock_acquire(&pid_lock);
	pid++;
	lock_release(&pid_lock);
	return pid;
}

/* sys_fork函数使用,子进程获取pid */
pid_t fork_pid(void) {
	return thread_allocate_pid();
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
	pthread->task.pid = thread_allocate_pid();
	pthread->task.ppid = (pid_t)-1;
	pthread->task.stack = (uint32_t)pthread + PAGE_SIZE;
	pthread->task.fd_table[0] = STDIN_FILENO;
	pthread->task.fd_table[1] = STDOUT_FILENO;
	pthread->task.fd_table[2] = STDERR_FILENO;
	for(uint32_t fd_index = 3;fd_index < MAX_FILES_OPEN_PRE_PROCESS;++fd_index)
		pthread->task.fd_table[fd_index] = -1;
	pthread->task.magic = 0x000055aa;

	if(strcmp(name, "main") != 0 && strcmp(name, "idle") != 0) // main线程创建根目录后其余线程才能使用 idle不需要
		pthread->task.cwd_inode_no = current_part->sb->root_inode_no;
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

/*
 @brief 线程阻塞
 @param status 线程要更改到的状态
 @note 线程阻塞是线程自己发起的动作 主动
 */
void thread_block(task_state_t status) {
	CLI_FUNC

	task_t *current = running_thread();
	current->state = status;
	schedule();

	STI_FUNC
}

/*
 @brief 解除线程阻塞状态
 @param pthread 线程指针
 @note 由持有临界资源的线程解除 被动
 */
void thread_unblock(task_t *pthread) {
	CLI_FUNC

	if(pthread->state != TASK_READY) {
		ASSERT(!list_find_item(&thread_ready_list, &pthread->general_list_item))
		list_push(&thread_ready_list, &pthread->general_list_item);
		pthread->state = TASK_READY;
	}

	STI_FUNC
}

/*
 @brief 主动让出cpu，进入就绪链表
 */
void thread_yield(void) {
	CLI_FUNC

	task_t *current = running_thread();
	ASSERT(!list_find_item(&thread_ready_list, &current->general_list_item));
	list_append(&thread_ready_list, &current->general_list_item);
	current->state = TASK_READY;
	schedule();

	STI_FUNC
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
	lock_init(&pid_lock);
	
	make_main_thread();
	idle_thread = (task_union_t *)thread_start("idle", 10, idle, NULL);
}
