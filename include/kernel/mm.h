#ifndef __MM_H__
#define __MM_H__

#include "../stdint.h"
#include "./bitmap.h"

#define PAGE_SIZE 4096

/* 内存分布 */
#define ARDS_ADDR 0x1002 //bios中断检测内存
#define ARDS_TIMES_ADDR 0x1000

#define OS_MEMORY_END 0x100000 //1M以下存放os代码
#define KERNEL_PHYSICS_MEMORY_MAP 0xC0090000 //放到堆栈下面 堆栈0x9F000 + 0xC0000000 堆栈处于1M内存最高处

typedef struct ards_item {
	uint32_t baseaddr_low32;
	uint32_t baseaddr_high32;
	uint32_t length_low32;
	uint32_t length_high32;
	uint32_t type;
}ards_item_t;

typedef struct check_memory_info {
	uint16_t times;
	ards_item_t *data;
}check_memory_info_t;

typedef enum pool_flag {
	pf_kernel = 0,
	pf_user,
}pool_flag_t;

typedef struct memory_pool {
	bitmap_t pool_bitmap; //位图管理
	uint32_t addr_start; //可用内存起始地址
	uint32_t valid_memory_size; //可用内存大小
	uint32_t addr_end; //可用内存结束地址
	uint32_t pages_total; //一共分多少页
	uint32_t pages_free; //空闲页数量
	uint32_t pages_used; //使用页数量
}memory_pool_t;
extern memory_pool_t kernel_physics_pool;
extern memory_pool_t user_physics_pool;

extern void print_check_memory_info(void);

extern void physics_memory_init(void);
extern void physics_memory_pool_init(void);
extern void *malloc_physics_page(memory_pool_t *pool);
extern void free_physics_page(memory_pool_t *pool, void *physics_addr);

#endif