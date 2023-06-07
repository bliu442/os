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

extern void k_thread_a(void *arg);
extern void k_thread_b(void *arg);
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

	hd_init();
	char buf[512] = {0};
	hd_read_sector(&channels[0].disk[0], 0, 1, buf);
	memset(buf, 0, sizeof(buf));
	hd_read_sector(&channels[0].disk[1], 0, 1, buf);
	memset(buf, 0, sizeof(buf));
	hd_read_sector(&channels[1].disk[0], 0, 1, buf); 
	memset(buf, 0, sizeof(buf));
	hd_read_sector(&channels[1].disk[1], 0, 1, buf);
	memset(buf, 0, sizeof(buf));

	memset(buf, 0x5A, sizeof(buf));
	hd_write_sector(&channels[0].disk[0], 0, 1, buf);
	hd_write_sector(&channels[0].disk[1], 0, 1, buf);
	hd_write_sector(&channels[1].disk[0], 0, 1, buf);
	hd_write_sector(&channels[1].disk[1], 0, 1, buf);

	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	thread_start("k_thread_b", 8, k_thread_b, "argB ");
	process_start(u_process_a, "u_process_a");

	STI
	out_byte(PIC_M_DATA, 0xFE); //打开时钟中断

	printk("main pid : %#x\r", sys_get_pid());
	while(1);
}

void k_thread_a(void *arg) {
	printk("k_thread_a pid : %#x\r", sys_get_pid());
	while(1);
}

void k_thread_b(void *arg) {
	printk("k_thread_b pid : %#x\r", sys_get_pid());
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