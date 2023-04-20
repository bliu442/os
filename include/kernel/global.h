#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "../stdint.h"

/* eflags寄存器 */
#define EFLAGS_MBS (1 << 1)
#define EFLAGS_IF (1 << 9)
#define EFLAGS_IF_1 (1 << 9)
#define EFLAGS_IF_0 (0 << 9)
#define EFLAGS_IOPL_3 (3 << 12)
#define EFLAGS_IOPL_0 (0 << 12)
#define GET_EFLAGS(EFLAGS_VAR) __asm__ __volatile__("pushfd; pop %0" : "=g" (EFLAGS_VAR))

#define DPL_0 (1)
#define DPL_3 (3)
#define PRESENT (1)
#define G_4K (1)
#define G_1B (0)
#define D_B_32 (1)
#define L_32 (0)
#define S_CODE_DATA (1)
#define S_SYS (0)
#define TYPE_INTERRUPT_GATE (0b1110)
#define TYPE_CODE (0b1000)
#define TYPE_DATA (0b0010)

//段选择子属性
#define SELECTOR_RPL_0 (0)
#define SELECTOR_RPL_3 (3)
#define SELECTOR_TI_GDT (0 << 2)

#define R0_CODE_SELECTOR (1 << 3 | SELECTOR_TI_GDT | SELECTOR_RPL_0)
#define R0_DATA_SELECTOR (2 << 3 | SELECTOR_TI_GDT | SELECTOR_RPL_0)
#define VIDEO_MEM_SELECTOR (3 << 3 | SELECTOR_TI_GDT | SELECTOR_RPL_0)
#define R3_CODE_SELECTOR (4 << 3 | SELECTOR_TI_GDT | SELECTOR_RPL_3)
#define R3_DATA_SELECTOR (5 << 3 | SELECTOR_TI_GDT | SELECTOR_RPL_3)

typedef struct gdt_item {
	uint16_t limit_low16;
	
	uint32_t baseaddr_low24 : 24;
	
	uint8_t type : 4; //段/门类型
	uint8_t segment : 1; //0:系统段 1:数据段
	uint8_t DPL : 2; //权限
	uint8_t present : 1; //内存中是否存在

	uint8_t limit_high8 : 4;
	uint8_t reserved : 1;
	uint8_t long_mode : 1; //0:32位 1:64位
	uint8_t d_b : 1; //1:32位 0:16位 (ip,eip,sp,esp)
	uint8_t granularity : 1; //粒度 1:4K 0:1B

	uint8_t baseaddr_high8;
}__attribute__((packed)) gdt_item_t;

typedef struct interrupt_gate {
	uint16_t offset_low16;

	uint16_t selector;
	
	uint8_t reserved;

	uint8_t type : 4; //1110:中断门
	uint8_t segment : 1; //0:系统段
	uint8_t DPL : 2;
	uint8_t present :1;

	uint16_t offset_high16;
}__attribute__((packed)) interrupt_gate_t;

typedef struct gdt_selector {
	uint8_t RPL : 2;
	uint8_t TI : 1;
	uint16_t index : 13;
}gdt_selector_t;

#pragma pack(2)
typedef struct xdt_ptr {
	uint16_t limit;
	uint32_t addr;
}xdt_ptr_t;
#pragma pack()

#endif