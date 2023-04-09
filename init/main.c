#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"
#include "../include/asm/io.h"

void _start(void) {
	put_str("\rkernel!\r");
	pic_init();
	idt_init();
	clock_init();

	STI
	out_byte(PIC_M_DATA, 0xFE); //打开时钟中断

	while(1);
}