#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"

void _start(void) {
	put_str("\rkernel!\r");
	pic_init();
	idt_init();

	STI
	int i = 10 / 0;

	while(1);
}