#include "../include/asm/system.h"
#include "../include/string.h"
#include "../include/kernel/traps.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/print.h"
#include "../include/kernel/process.h"

// #define GDT_SIZE 8192
#define GDT_SIZE 256

uint64_t gdt[GDT_SIZE];

xdt_ptr_t gdt_ptr;

tss_t tss; //所有任务使用同一个tss,只用r0堆栈,切换任务时加载不同任务的r0堆栈

/*
 @brief 向gdt表中添加一个r3代码段描述符
 @param gdt_index 索引值
 @param addr 段基址
 @param limit 段界限 粒度4K
 */
static void r3_gdt_code_item(uint32_t gdt_index, uint32_t addr, uint32_t limit) {
	ASSERT(gdt_index >= 4);

	gdt_item_t* item = (gdt_item_t *)&gdt[gdt_index];

	item->limit_low16 = limit & 0xFFFF;
	item->baseaddr_low24 = addr & 0xFFFFFF;
	item->type = TYPE_CODE;
	item->segment = S_CODE_DATA;
	item->DPL = DPL_3;
	item->present = PRESENT;
	item->limit_high8 = limit >> 16 & 0xF;
	item->reserved = 0;
	item->long_mode = L_32;
	item->d_b = D_B_32;
	item->granularity = G_4K;
	item->baseaddr_high8 = addr >> 24 & 0xFF;
}

/*
 @brief 向gdt表中添加一个r3数据段描述符
 @param gdt_index 索引值
 @param addr 段基址
 @param limit 段界限 粒度4K
 */
static void r3_gdt_data_item(uint32_t gdt_index, uint32_t addr, uint32_t limit) {
	ASSERT(gdt_index >= 4);

	gdt_item_t* item = (gdt_item_t *)&gdt[gdt_index];

	item->limit_low16 = limit & 0xFFFF;
	item->baseaddr_low24 = addr & 0xFFFFFF;
	item->type = TYPE_DATA;
	item->segment = S_CODE_DATA;
	item->DPL = DPL_3;
	item->present = PRESENT;
	item->limit_high8 = limit >> 16 & 0xF;
	item->reserved = 0;
	item->long_mode = L_32;
	item->d_b = D_B_32;
	item->granularity = G_4K;
	item->baseaddr_high8 = addr >> 24 & 0xFF;
}

/*
 @brief 向gdt表中添加一个tss描述符
 @param gdt_index 索引值
 @param addr 段基址
 @param limit 段界限 粒度1B
 */
static void gdt_tss_item(uint32_t gdt_index, uint32_t addr, uint32_t limit) {
	uint32_t tss_size = sizeof(tss_t);
	memset(&tss, 0, tss_size);
	tss.ss0 = R0_DATA_SELECTOR; //r0数据段
	tss.io_base = tss_size;

	gdt_item_t* item = (gdt_item_t *)&gdt[gdt_index];

	item->limit_low16 = limit & 0xFFFF;
	item->baseaddr_low24 = addr & 0xFFFFFF;
	item->type = TYPE_TSS;
	item->segment = S_SYS;
	item->DPL = DPL_0;
	item->present = PRESENT;
	item->limit_high8 = limit >> 16 & 0xF;
	item->reserved = 0;
	item->long_mode = L_32;
	item->d_b = D_B_32;
	item->granularity = G_1B;
	item->baseaddr_high8 = addr >> 24 & 0xFF;
}

/*
 @brief 初始化gdt表 
 */
void gdt_init(void) {
	put_str("init gdt ... \r");

	__asm__("sgdt gdt_ptr;");

	memcpy(&gdt, (void *)gdt_ptr.addr, gdt_ptr.limit);

	r3_gdt_code_item(R3_CODE_SELECTOR >> 3, 0, 0xFFFFF);
	r3_gdt_data_item(R3_DATA_SELECTOR >> 3, 0, 0xFFFFF);
	gdt_tss_item(TSS_SELECTOR >> 3, (uint32_t)&tss, sizeof(tss_t) - 1);

	gdt_ptr.addr = (uint32_t)&gdt[0];
	gdt_ptr.limit = sizeof(gdt) - 1;

	BOCHS_DEBUG_MAGIC;
	__asm__("lgdt gdt_ptr;");

	__asm__("mov ax, %0; ltr ax" :: "memory" (TSS_SELECTOR));
}