#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"
#include "../include/asm/io.h"
#include "../include/string.h"
#include "../include/kernel/mm.h"

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

	uint32_t *ptr = malloc_kernel_page(2);
	free_kernel_page(ptr, 2);

	while(1);

	STI
	out_byte(PIC_M_DATA, 0xFE); //打开时钟中断

	while(1);
}