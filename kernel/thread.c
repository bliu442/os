/*
 thread process task
 thread process都是task 任务
 */

#include "../include/kernel/thread.h"
#include "../include/kernel/mm.h"
#include "../include/string.h"

/*
 @brief 执行线程函数
 @param function 线程函数
 @param func_arg 线程函数参数
 */
static void kernel_thread(thread_fun_t *function, void *func_arg) {
	function(func_arg);
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
	strcpy(pthread->task.name, name);
	pthread->task.state = TASK_RUNNING;
	pthread->task.priority = priority;
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

 @note ret调用函数
 |func_arg|
 |function|
 |.retaddr|

 |.....eip| <-ret 将eip当作retaddr弹出到eip寄存器
 |.....esi|
 |.....edi|
 |.....ebx|
 |.....ebp| <-esp

 1.将esp赋值为task.stack
 2.pop四个数据到相应寄存器,esp指向eip
 3.ret 将eip(kernel_thread)当作返回地址,传给eip寄存器,执行线程函数
 目前esp指向retaddr 虽然kernel_thread是靠ret进入的,但执行时还是按照C语言规范 认为esp为返回地址 esp+4为第一个参数 esp+8为第二个参数
 4.取到参数function func_arg,开始执行线程函数
 */
task_union_t *thread_start(char *name, uint32_t priority, thread_fun_t function, void *func_arg) {
	task_union_t *pthread = malloc_kernel_page(1);
	memset(pthread, 0, PAGE_SIZE);

	thread_init(pthread, name, priority);
	thread_create(pthread, function, func_arg);

	__asm__("mov esp, %0;"
			"pop ebp;"
			"pop ebx;"
			"pop edi;"
			"pop esi;"
			"ret"
			:
			:"g" (pthread->task.stack)
		);
}
