#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"
#include "../include/asm/io.h"
#include "../include/string.h"
#include "../include/kernel/mm.h"
#include "../include/kernel/thread.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/process.h"
#include "../include/unistd.h"
#include "../include/kernel/syscall.h"
#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/hd.h"
#include "../include/kernel/shell.h"

extern void u_process_a(void);

void _start(void) {
	put_str("\rkernel!\r");
	pic_init();
	idt_init();
	gdt_init();
	clock_init();

	print_check_memory_info();
	physics_memory_init();
	physics_memory_pool_init();

	virtual_memory_init();
	virtual_memory_pool_init();

	pthread_init();

	console_init(); //之后使用C语言写的显卡驱动
	printk_init();

	shell_init();
	hd_init();

	thread_start("shell", 31, shell, NULL);
	// process_start(u_process_a, "u_process_a");

	STI
	out_byte(PIC_M_DATA, 0b11111000); //打开0时钟中断 1键盘中断 2号级联
	out_byte(PIC_S_DATA, 0b00111111); //打开两个硬盘中断 14 15

	printk("main pid : %#x\r", sys_get_pid());
	while(1);
}

void u_process_a(void) {
	pid_t pid = get_pid();

	printf("u_process_a pid : %#x\r", pid);

	pid_t child_pid = fork();
	if(child_pid == 5) {
		INFO("parent\r");
	} else {
		INFO("child\r");
	}
	
	INFO("two print\r");

	while(1);
}