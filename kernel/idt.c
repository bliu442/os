#include "../include/asm/system.h"
#include "../include/kernel/global.h"
#include "../include/kernel/print.h"

extern int interrupt_handler_table[0x30]; //see interrupt_handler.asm
extern void interrupt_handler_default(void); //see interrupt_handler.asm

#define IDT_SIZE 256

interrupt_gate_t idt[IDT_SIZE];

xdt_ptr_t idt_ptr;

/* @brief 初始化idt表 */
void idt_init(void) {
	put_str("init idt ... \r");

	for(int i = 0;i < IDT_SIZE;++i) {
		interrupt_gate_t *ptr = &idt[i];

		int handler = (uint32_t)interrupt_handler_default;

		if(i < 0x30)
			handler = (uint32_t)interrupt_handler_table[i];

		ptr->offset_low16 = handler & 0xFFFF;
		ptr->selector = R0_CODE_SELECTOR;
		ptr->reserved = 0;
		ptr->type = TYPE_INTERRUPT_GATE;
		ptr->segment = S_SYS;
		ptr->DPL = DPL_0;
		ptr->present = PRESENT;
		ptr->offset_high16 = handler >> 16 & 0xFFFF;
	}

	idt_ptr.addr = (uint32_t)&idt[0];
	idt_ptr.limit = sizeof(idt) - 1;

	BOCHS_DEBUG_MAGIC
	__asm__("lidt idt_ptr;");
}