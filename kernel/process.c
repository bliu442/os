/*
 用户进程
 process比thread多了cr3和虚拟内存池
 cr3用于管理进程虚拟内存,切换cr3实现进程的地址隔离,使每个进程独享4G虚拟内存
 虚拟内存池用于管理进程堆的动态申请
 .text .bss .data stack由操作系统加载可执行程序(创建进程)时分配
 */

#include "../include/kernel/process.h"
#include "../include/kernel/mm.h"
#include "../include/kernel/global.h"
#include "../include/string.h"
#include "../include/asm/system.h"
#include "../include/kernel/print.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/traps.h"

#define USER_VIRTUAL_MEMORY_START 0xB0000000 //暂定
#define USER_STACK 0xC0000000

/*
 @brief 构建用户进程上下文堆栈 使用中断返回执行进程函数,回到到用户态
 @param filename 先用使用函数(.elf)
 @note 创建一个task,先进入内核线程,在线程中构建用户进程上下文堆栈,使用中断返回回到用户态
 */
void process_execute(void *filename) {
	void *function = filename;

	task_t *current = running_thread();

	current->stack += sizeof(thread_stack_t);
	interrupt_stack_t *process_stack = (interrupt_stack_t *)(current->stack);
	/*
	 使用中断返回回到r3 构建中断堆栈
	 1.提前构建好堆栈结构,填充用户进程上下文信息，为用户进程执行准备好环境
	 2.置位eflags相应的位 伪造中断
	 |....ss_old|
	 |...esp_old|
	 |....eflags|
	 |........cs|
	 |.......eip| iret
	 |.......eax|
	 |.......ecx|
	 |.......edx|
	 |.......ebx|
	 |.......esp|
	 |.......ebp|
	 |.......esi|
	 |.......edi|
	 |........ds|
	 |........es|
	 |........fs|
	 |........gs| <-process_stack
	*/
	process_stack->edi = process_stack->esi = process_stack->ebp = process_stack->esp = 0;
	process_stack->ebx = process_stack->edx = process_stack->ecx = process_stack->eax = 0;
	process_stack->gs = process_stack->fs = process_stack->es = process_stack->ds = R3_DATA_SELECTOR;
	process_stack->eip = (uint32_t)function;
	process_stack->cs = R3_CODE_SELECTOR;
	process_stack->eflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
	process_stack->esp_old = (uint32_t)malloc_a_page(pf_user, USER_STACK - PAGE_SIZE) + PAGE_SIZE; //将r3堆栈栈顶设置为0xC0000000
	process_stack->ss_old = R3_DATA_SELECTOR;

	// BOCHS_DEBUG_MAGIC //查看堆栈
	// BOCHS_DEBUG_MAGIC

	__asm__("mov esp, %0;"
		"pop gs;"
		"pop fs;"
		"pop es;"
		"pop ds;"
		"popad;"
		"add esp, 8;"
		"iret"
		:: "g" (process_stack): "memory");
}

/*
 @brief 创建用户进程页目录表
 @retval pdt虚拟地址
 @note pdt管理的是当前用户进程虚拟内存,存放在内核内存空间中
 */
uint32_t process_create_pdt(void) {
	uint32_t *pdt_virtual_addr = malloc_kernel_page(1);
	if(pdt_virtual_addr == NULL) {
		printk("%s : malloc_kernel_page failed!\r", __func__);
		return -1;
	}

	// BOCHS_DEBUG_MAGIC
	// BOCHS_DEBUG_MAGIC

	/* 拷贝内核页目录表 */
	memcpy(pdt_virtual_addr, (uint32_t *)(0xFFFFF000), 4); //0-4M
	memcpy((void *)((uint32_t)pdt_virtual_addr + 0x300 * 4), (uint32_t *)(0xFFFFF000 + 0x300 * 4), 1024); //0xC0000000
	
	uint32_t pdt_physics_addr = addr_virtual2physics((uint32_t)pdt_virtual_addr);
	pdt_virtual_addr[1023] = (uint32_t)pdt_physics_addr | PAGE_P | PAGE_RW_W | PAGE_US_U;

	return (uint32_t)pdt_virtual_addr; //pcb->cr3中存储的是pdt的虚拟地址,方便程序中使用,写入cr3寄存器时需要转换成物理地址
}

/*
 @brief 创建用户进程虚拟地址内存池
 @process 当前进程
 @note 虚拟内存池管理的是当前用户进程堆的动态申请,存放在内核内存空间中
 */
void create_user_virtual_memory_pool(task_t *process) {
	process->user_virtual_pool.addr_start = USER_VIRTUAL_MEMORY_START;
	process->user_virtual_pool.valid_memory_size = USER_STACK - PAGE_SIZE - process->user_virtual_pool.addr_start;
	process->user_virtual_pool.addr_end = process->user_virtual_pool.addr_start + process->user_virtual_pool.valid_memory_size;
	process->user_virtual_pool.pages_total = process->user_virtual_pool.valid_memory_size / PAGE_SIZE;
	process->user_virtual_pool.pages_used = 0;
	process->user_virtual_pool.pages_free = process->user_virtual_pool.pages_total - process->user_virtual_pool.pages_used;

	process->user_virtual_pool.pool_bitmap.bitmap_byte_len = process->user_virtual_pool.pages_total / 8;
	// if(process->user_virtual_pool.pages_total % 8)
	// 	process->user_virtual_pool.pool_bitmap.bitmap_byte_len++;
	uint32_t bitmap_page_count = DIV_ROUND_UP(process->user_virtual_pool.pool_bitmap.bitmap_byte_len, PAGE_SIZE);
	process->user_virtual_pool.pool_bitmap.map = malloc_kernel_page(bitmap_page_count);
	if(process->user_virtual_pool.pool_bitmap.map == NULL) {
		printk("%s : malloc_kernel_page failed!\r", __func__);
		return;
	}

	bitmap_init(&process->user_virtual_pool.pool_bitmap);
}

/*
 @brief 初始化进程内存管理模块
 */
void user_bucket_dir_init(task_t *process) {
	uint32_t len = sizeof(process->user_bucket_dir) / sizeof(process->user_bucket_dir[0]);
	uint32_t loop = 0;
	uint32_t size = 16;

	for(loop = 0;loop < len - 1;++loop) {
		process->user_bucket_dir[loop].size = size;
		process->user_bucket_dir[loop].chain = NULL;
		size *= 2;
	}

	process->user_bucket_dir[9].size = 0;
	process->user_bucket_dir[9].chain = NULL;
}

/* 
 @brief 创建用户进程
 @note pcb由内核维护
 */
void process_start(void *filename, char *name) {
	task_union_t *process = malloc_kernel_page(1);
	if(process == NULL) {
		printk("%s : malloc_kernel_page failed!\r\n", __func__);
		return;
	}

	thread_init(process, name, 31);
	thread_create(process, process_execute, filename);

	process->task.cr3 = process_create_pdt();
	create_user_virtual_memory_pool((task_t *)process);
	user_bucket_dir_init((task_t *)process);

	CLI_FUNC
	ASSERT(!list_find_item(&thread_ready_list, &process->task.general_list_item));
	list_append(&thread_ready_list, &process->task.general_list_item);
	
	ASSERT(!list_find_item(&thread_all_list, &process->task.all_list_item));
	list_append(&thread_all_list, &process->task.all_list_item);

	STI_FUNC
}

/*
 @brief 切换进程r0堆栈 所有进程共用一个tss段描述符
 @process 当前进程
 */
void process_tss_activate(task_t *process) {
	tss.esp0 = (uint32_t)process + PAGE_SIZE;
}

/*
 @brief 切换进程/线程内存空间
 @process 当前进程
 */
void process_pdt_activate(task_t *process) {
	uint32_t pdt_physics_addr = OS_PAGE_DIR_TABLE_POS; //内核线程
	if(process->cr3 != 0) //用户进程
		pdt_physics_addr = addr_virtual2physics(process->cr3);

	__asm__ volatile("mov cr3, %0" :: "r" (pdt_physics_addr) : "memory");
}

/*
 @brief 切换进程内存空间/堆栈
 @process 当前进程
 */
void process_activate(task_t *process) {
	process_pdt_activate(process);

	if(process->cr3)
		process_tss_activate(process); //中断发生时如果在r3,cpu自动到tss里拿到r0的堆栈地址
}
