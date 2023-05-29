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
	printk("main.c %x\r", RED, 0x55);
	printk("main.c %x\r", 0x55);

	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	thread_start("k_thread_b", 8, k_thread_b, "argB ");
	process_start(u_process_a, "u_process_a");

	uint32_t cs_reg = 0;
	GET_CS(cs_reg);
	ERROR("main.c %x\r", 0x55);
	WARN("main.c %x\r", 0x55);
	INFO("main.c %x\r", 0x55);

	STI
	out_byte(PIC_M_DATA, 0xFE); //打开时钟中断

	printk("main pid : %#x\r", sys_get_pid());
	while(1);
}

void k_thread_a(void *arg) {
	printk("k_thread_a pid : %#x\r", sys_get_pid());

	while(1) {
		uint8_t *p1 = kmalloc(3);
		uint8_t *p2 = kmalloc(3);
		uint8_t *p3 = kmalloc(3);
		kfree(p1, 3);
		kfree(p2, 3);
		uint8_t *p4 = kmalloc(23);
		uint8_t *p5 = kmalloc(23);
		uint8_t *p6 = kmalloc(23);
		kfree(p3, 3);
		kfree(p4, 23);
		kfree(p5, 23);
		kfree(p6, 23);
	}

	while(1);
}

void k_thread_b(void *arg) {
	printk("k_thread_b pid : %#x\r", sys_get_pid());
	while(1);
}

void u_process_a(void) {
	pid_t pid = get_pid();

	printf("u_process_a pid : %#x\r", pid);
	uint32_t cs_reg = 0;
	GET_CS(cs_reg);
	ERROR("main.c %x\r", 0x55);
	WARN("main.c %x\r", 0x55);
	INFO("main.c %x\r", 0x55);

	while(1) {
		uint8_t *p1 = malloc(3);
		uint8_t *p2 = malloc(3);
		uint8_t *p3 = malloc(3);
		free(p1, 3);
		free(p2, 3);
		uint8_t *p4 = malloc(23);
		uint8_t *p5 = malloc(23);
		uint8_t *p6 = malloc(23);
		free(p3, 3);
		free(p4, 23);
		free(p5, 23);
		free(p6, 23);
	}
	while(1);
}