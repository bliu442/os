#include "../include/unistd.h"
#include "../include/string.h"
#include "../include/kernel/process.h"
#include "../include/asm/system.h"
#include "../include/kernel/file.h"
#include "../include/kernel/fs.h"

#define TAG "fork"
#include "../include/kernel/debug.h"

extern void interrupt_exit(void);

// pid_t fork(void) {
// 	pid_t ret = 0;

// 	__asm__ volatile("int 0x80"
// 		: "=a" (ret)
// 		: "0" (__NR_fork)
// 	);

// 	return ret;
// }

/*
 @param 复制父进程pcb virtual_addr_bitmap stack0 到子进程
 @param child_thread 子进程pcb
 @param parent_thread 父进程pcb
 */
static int32_t copy_pcb_virtual_addr_bitmap_stack0(task_t *child_thread, task_t *parent_thread) {
	//1.pcb stack0
	memcpy(child_thread, parent_thread, PAGE_SIZE);
	if(strlen(child_thread->name) < 25) {
		strcat(child_thread->name, "_fork");
	}
	child_thread->state = TASK_READY;
	child_thread->ticks = child_thread->priority;
	child_thread->elapsed_ticks = 0;
	child_thread->pid = fork_pid();
	child_thread->ppid = parent_thread->pid;
	child_thread->general_list_item.next = child_thread->general_list_item.prev = NULL;
	child_thread->all_list_item.next = child_thread->all_list_item.prev = NULL;
	user_bucket_dir_init(child_thread);

	//2.virtual_addr_bitmap 创建bitmap,拷贝父进程bitmap,加载进子进程pcb
	uint32_t bitmap_page_count = DIV_ROUND_UP(child_thread->user_virtual_pool.pool_bitmap.bitmap_byte_len, PAGE_SIZE);
	void *virtual_pool_bitmap = malloc_kernel_page(bitmap_page_count);
	if(virtual_pool_bitmap == NULL) {
		INFO("malloc_kernel_page failed!\r");
		return -1;
	}
	memcpy(virtual_pool_bitmap, child_thread->user_virtual_pool.pool_bitmap.map, bitmap_page_count * PAGE_SIZE);
	child_thread->user_virtual_pool.pool_bitmap.map = virtual_pool_bitmap;

	return 0;
}

/*
 @brief 将父进程bitmap中所有页拷贝到子进程中
 @param child_thread 子进程pcb
 @param parent_thread 父进程pcb
 @param buf 内核缓冲区
 */
static void copy_body_stack3(task_t *child_thread, task_t *parent_thread, void *buf) {
	uint8_t *virtual_memory_bitmap = parent_thread->user_virtual_pool.pool_bitmap.map;
	uint32_t bitmap_byte_len = parent_thread->user_virtual_pool.pool_bitmap.bitmap_byte_len;
	uint32_t addr_start = parent_thread->user_virtual_pool.addr_start;
	uint32_t byte_index = 0;
	uint32_t bit_index = 0;
	uint32_t addr = 0;

	//1.查找父进程已经使用的页
	while(byte_index <= bitmap_byte_len) {

	/*
	 mark bitmap初始化时,少追踪了一个字节,申请r3堆栈时bitmap置位了最后一个字节 bug 要用<= 要不拷贝不到r3堆栈
	 if(process->user_virtual_pool.pages_total % 8)
		 process->user_virtual_pool.pool_bitmap.bitmap_byte_len++;

	*/
		if(virtual_memory_bitmap[byte_index]) {
			bit_index = 0;
			while(bit_index < 8) {
				if(virtual_memory_bitmap[byte_index] & (bitmap_used << bit_index)) { //有内存被使用
					//2.拷贝 找到虚拟地址 拷贝到内核空间 切子进程页表 申请一页内存 拷贝到子进程空间 切父进程页表
					addr = (byte_index * 8 + bit_index) * PAGE_SIZE + addr_start;

					memcpy(buf, (void *)addr, PAGE_SIZE);

					// BOCHS_DEBUG_MAGIC //1.0xBFFFF000 父进程r3堆栈数据
					// BOCHS_DEBUG_MAGIC

					process_pdt_activate(child_thread);

					addr = (uint32_t)malloc_a_page_without_add_virtual_bitmap(pf_user, addr);
					if(addr == 0) {
						INFO("malloc_a_page_without_add_virtual_bitmap\r");
						return; //mark 内存回收
					}

					// BOCHS_DEBUG_MAGIC //2.0xBFFFF000 新创建物理页,page table应该改变
					// BOCHS_DEBUG_MAGIC

					memcpy((void *)addr, buf, PAGE_SIZE);

					// BOCHS_DEBUG_MAGIC //3.0xBFFFF000 将父进程堆栈拷贝
					// BOCHS_DEBUG_MAGIC

					process_pdt_activate(parent_thread);
				}
				bit_index++;
			}
		}
		byte_index++;
	}
}

/*
 @prief 构建r0堆栈,使子进程返回时能够从fork之后的代码处运行

 @note 用户进程->fork->软中断->sys_fork
 父进程执行完sys_fork后中断返回
 在sys_fork里子进程被创建添加进进程链表中,但不是新创建的进程,它的上下文是父进程软中断的状态
 在该上下文的基础上,构造进程调度的堆栈
 */

/*
 软中断堆栈
 |..eflags| 进入中断时保存的上下文 第一层上下文
 |......cs|
 |.....eip| 在此处拿到发生中断时执行的函数,回到线程中
 |...error|
 |...index|
 |.....eax|
 |.....ecx|
 |.....edx|
 |.....ebx|
 |.....esp| popad忽略esp
 |.....ebp|
 |.....esi|
 |.....edi|
 |......ds|
 |......es|
 |......fs|
 |......gs|
 ------------------- stack0拷贝时堆栈,从此处以下对子进程无用
 |.....edx|
 |.....ecx|
 |.....ebx|
 |.retaddr| call syscall
 -------------------
 |........|
 */
static int32_t build_child_stack(task_t *child_thread) {
	//1.子进程使用父进程中断上下文 更改eax使fork子进程返回值为0
	interrupt_stack_t *interrupt_stack = (interrupt_stack_t *)((uint32_t)child_thread + PAGE_SIZE - sizeof(interrupt_stack_t));
	interrupt_stack->eax = 0;

	/*
	 2.
	 父进程由中断自动返回
	 子进程添加进进程链表,由switch_to调度 调度开始时刻为中断返回
	 */
	uint32_t *retaddr_in_interrupt_stack = (uint32_t *)interrupt_stack - 1;
	uint32_t *esi_in_interrupt_stack = (uint32_t *)interrupt_stack - 2;
	uint32_t *edi_in_interrupt_stack = (uint32_t *)interrupt_stack - 3;
	uint32_t *ebx_in_interrupt_stack = (uint32_t *)interrupt_stack - 4;
	uint32_t *ebp_in_interrupt_stack = (uint32_t *)interrupt_stack - 5;

	*esi_in_interrupt_stack = *edi_in_interrupt_stack = *ebx_in_interrupt_stack = *ebp_in_interrupt_stack = 0;

	*retaddr_in_interrupt_stack = (uint32_t)interrupt_exit;

	child_thread->stack = (uint32_t)ebp_in_interrupt_stack;
	return 0;
}

static void update_inode_open_counts(task_t *thread) {
	int32_t local_fd = 3;
	int32_t global_fd = 0;
	while(local_fd < MAX_FILES_OPEN_PRE_PROCESS) {
		global_fd = thread->fd_table[local_fd];
		if(global_fd != -1)
			file_table[global_fd].fd_inode->i_open_counts++;

		local_fd++;
	}
}

static int32_t copy_process(task_t *child_thread, task_t *parent_thread) {
	void *buf = malloc_kernel_page(1); //内核缓冲区
	if(buf == NULL) {
		INFO("malloc_kernel_page\r");
		return -1;
	}

	if(copy_pcb_virtual_addr_bitmap_stack0(child_thread, parent_thread) == -1) {
		goto fail;
	}

	child_thread->cr3 = process_create_pdt();
	if((int32_t)child_thread->cr3 == -1) {
		goto fail;
	}

	copy_body_stack3(child_thread, parent_thread, buf);

	build_child_stack(child_thread);

	update_inode_open_counts(child_thread);

	// BOCHS_DEBUG_MAGIC
	// BOCHS_DEBUG_MAGIC //debug
	// process_pdt_activate(child_thread);
	// BOCHS_DEBUG_MAGIC
	// BOCHS_DEBUG_MAGIC
	// process_pdt_activate(parent_thread);
	// BOCHS_DEBUG_MAGIC
	// BOCHS_DEBUG_MAGIC

	free_kernel_page(buf, 1);
	return 0;

fail:
	free_kernel_page(buf, 1);
	return -1;
}

pid_t sys_fork(void) {
	task_t *parent_thread = running_thread();
	task_t *child_thread = malloc_kernel_page(1);
	if(child_thread == NULL) {
		INFO("malloc_kernel_page\r");
		return (pid_t)-1;
	}

	if(copy_process(child_thread, parent_thread) == -1) {
		free_kernel_page(child_thread, 1);
		return (pid_t)-1;
	}

	ASSERT(!list_find_item(&thread_ready_list, &child_thread->general_list_item));
	list_append(&thread_ready_list, &child_thread->general_list_item);
	
	ASSERT(!list_find_item(&thread_all_list, &child_thread->all_list_item));
	list_append(&thread_all_list, &child_thread->all_list_item);

	return child_thread->pid;
}