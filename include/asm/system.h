#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "../stdint.h"
#include "../kernel/interrupt.h"

#define BOCHS_DEBUG_MAGIC __asm__("xchg bx, bx"); //bochs 调试断点

#define STI __asm__("sti"); //开中断
#define CLI __asm__("cli"); //关中断

#define CLI_FUNC if_enum_t old_status = interrupt_disable();
#define STI_FUNC interrupt_set_status(old_status);

#endif