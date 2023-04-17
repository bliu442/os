#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"
#include "../include/asm/io.h"
#include "../include/string.h"
#include "../include/kernel/mm.h"
#include "../include/kernel/thread.h"

extern void k_thread_a(void *arg);
extern void k_thread_b(void *arg);

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

	pthread_init();

	console_init();

	thread_start("k_thread_a", 31, k_thread_a, "argA ");
	thread_start("k_thread_b", 8, k_thread_b, "argB ");

	STI
	out_byte(PIC_M_DATA, 0xFE); //打开时钟中断

	uint32_t i = 0x100000;
	while(true) {
		console_put_str("main ");
		while(i--) {
			__asm__("nop;");
		}
		i = 0x100000;
	}
}

void k_thread_a(void *arg) {
	char *param = arg;

	uint32_t i = 0x100000;
	while(true) {
		console_put_str(param);
		while(i--) {
			__asm__("nop;");
		}
		i = 0x100000;
	}
}

void k_thread_b(void *arg) {
	char *param = arg;
	uint32_t i = 0x100000;
	while(true) {
		console_put_str(param);
		while(i--) {
			__asm__("nop;");
		}
		i = 0x100000;
	}
}