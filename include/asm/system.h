#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include "../stdint.h"

#define BOCHS_DEBUG_MAGIC __asm__("xchg bx, bx"); //bochs 调试断点

#define STI __asm__("sti"); //开中断
#define CLI __asm__("cli"); //关中断

#endif