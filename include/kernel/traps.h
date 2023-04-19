#ifndef __TRAPS_H__
#define __TRAPS_H__

#include "./global.h"

extern xdt_ptr_t idt_ptr;
extern xdt_ptr_t gdt_ptr;
extern void idt_init(void);
extern void gdt_init(void);

#endif