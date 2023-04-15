#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"
#include "../include/asm/io.h"
#include "../include/string.h"
#include "../include/kernel/mm.h"
#include "../include/kernel/thread.h"

extern void k_thread_a(void *arg);

void _start(void) {
	put_str("\rkernel!\r");
	pic_init();
	idt_init();
	clock_init();

	print_check_memory_info();
	physics_memory_init();
	physics_memory_pool_init();

	virtual_memory_init();
	virtual_memory_pool_init();

	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	while(1);

	STI
	out_byte(PIC_M_DATA, 0xFE); //打开时钟中断

	while(1);
}

void k_thread_a(void *arg) {
	char *param = arg;
	while(true)
		put_str(param);
}