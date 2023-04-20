#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "../stdint.h"

#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xA0
#define PIC_S_DATA 0xA1

typedef enum if_enmu {
	if_cli, //0 关中断
	if_sti, //1 开中断
}if_enum_t;

extern void pic_init(void);
extern void clock_init(void);

extern void exception_handler(uint32_t gs, uint32_t fs, uint32_t es, uint32_t ds, uint32_t edi, uint32_t esi,
	uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax,
	uint32_t idt_index, uint32_t error_code, uint32_t eip, uint32_t cs, uint32_t eflags);
extern void clock_handler(void);

extern if_enum_t interrupt_disable(void);
extern if_enum_t interrupt_set_status(if_enum_t status);

#endif