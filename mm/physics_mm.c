/*
 物理内存管理
 将物理内存分为内核和用户两个区域
 内核申请内存从内核池中分配
 用户申请堆从用户池中分配
 */

#include "../include/kernel/mm.h"
#include "../include/string.h"
#include "../include/kernel/print.h"
#include "../include/kernel/debug.h"

#define ZONE_VALID 1
#define ZONE_RESERVED 2

memory_pool_t kernel_physics_pool;
memory_pool_t user_physics_pool;

/* @brief 打印bios int 0x15中断获取的物理内存容量 */
void print_check_memory_info(void) {
	check_memory_info_t *p = (check_memory_info_t *)ARDS_TIMES_ADDR;
	ards_item_t *p_ards = (ards_item_t *)ARDS_ADDR;

	put_str("memory check info\r");
	for(uint8_t i = 0;i < p->times;++i) {
		ards_item_t *p_tmp = p_ards + i;

		put_str("memory start:");
		// put_int(p_tmp->baseaddr_high32);
		// put_char('-');
		put_int(p_tmp->baseaddr_low32);

		put_str("    memory len:");
		// put_int(p_tmp->length_high32);
		// put_char('-');
		put_int(p_tmp->length_low32);
		
		put_str("    memory type:");
		put_int(p_tmp->type);
		put_char('\r');
	}
}

/* @brief 挑选可用内存 */
void physics_memory_init(void) {
	check_memory_info_t *p = (check_memory_info_t *)ARDS_TIMES_ADDR;
	ards_item_t *p_ards = (ards_item_t *)ARDS_ADDR;

	memory_pool_t physics_memory = {0};

	for(uint8_t i = 0;i < p->times;++i) {
		ards_item_t *p_tmp = p_ards + i;

		if(p_tmp->type == ZONE_VALID && p_tmp->baseaddr_low32 >= OS_MEMORY_END) {
			physics_memory.addr_start = p_tmp->baseaddr_low32;
			physics_memory.valid_memory_size = p_tmp->length_low32;
		}
	}

	kernel_physics_pool.addr_start = physics_memory.addr_start;
	kernel_physics_pool.valid_memory_size = physics_memory.valid_memory_size / 2;
	kernel_physics_pool.addr_end = kernel_physics_pool.addr_start + kernel_physics_pool.valid_memory_size;

	user_physics_pool.addr_start = kernel_physics_pool.addr_end;
	user_physics_pool.valid_memory_size = physics_memory.valid_memory_size / 2;
	user_physics_pool.addr_end = user_physics_pool.addr_start + user_physics_pool.valid_memory_size;
	
	kernel_physics_pool.pages_total = kernel_physics_pool.valid_memory_size >> 12;
	kernel_physics_pool.pages_used = 256;
	kernel_physics_pool.pages_free = kernel_physics_pool.pages_total - kernel_physics_pool.pages_used;

	user_physics_pool.pages_total = user_physics_pool.valid_memory_size >> 12;
	user_physics_pool.pages_used = 0;
	user_physics_pool.pages_free = user_physics_pool.pages_total - user_physics_pool.pages_used;

	put_str("kernel physical memory position :");
	put_int(kernel_physics_pool.addr_start);
	put_char('-');
	put_int(kernel_physics_pool.addr_end);
	put_str("    used : ");
	put_int(kernel_physics_pool.pages_used);
	put_str("page\r");

	put_str("user physical memory position :");
	put_int(user_physics_pool.addr_start);
	put_char('-');
	put_int(user_physics_pool.addr_end);
	put_str("    used : ");
	put_int(user_physics_pool.pages_used);
	put_str("page\r");
}


/* @brief 实现物理内存管理方案 bitmap使用1位表示1页物理内存是否被使用 */
void physics_memory_pool_init(void) {
	ASSERT(kernel_physics_pool.valid_memory_size != 0);
	ASSERT(user_physics_pool.valid_memory_size != 0);

	kernel_physics_pool.pool_bitmap.bitmap_byte_len = kernel_physics_pool.pages_total >> 3;
	// if(kernel_physics_pool.pages_total % 8) //丢掉一些内存
	// 	kernel_physics_pool.pool_bitmap.bitmap_byte_len++;
	kernel_physics_pool.pool_bitmap.map = (uint8_t *)KERNEL_PHYSICS_MEMORY_MAP;

	bitmap_init(&kernel_physics_pool.pool_bitmap);

	user_physics_pool.pool_bitmap.bitmap_byte_len = user_physics_pool.pages_total >> 3;
	// if(user_physics_pool.pages_total % 8)
	// 	user_physics_pool.pool_bitmap.bitmap_byte_len++;
	user_physics_pool.pool_bitmap.map = (uint8_t *)KERNEL_PHYSICS_MEMORY_MAP + kernel_physics_pool.pool_bitmap.bitmap_byte_len;

	put_str("kernel physics memory pool position: ");
	put_int((uint32_t)kernel_physics_pool.pool_bitmap.map);
	put_char('-');
	put_int((uint32_t)kernel_physics_pool.pool_bitmap.map + kernel_physics_pool.pool_bitmap.bitmap_byte_len);
	put_char('\r');

	put_str("user physics memory pool position: ");
	put_int((uint32_t)user_physics_pool.pool_bitmap.map);
	put_char('-');
	put_int((uint32_t)user_physics_pool.pool_bitmap.map + user_physics_pool.pool_bitmap.bitmap_byte_len);
	put_char('\r');

	bitmap_init(&kernel_physics_pool.pool_bitmap);
	bitmap_init(&user_physics_pool.pool_bitmap);

	/* 已经使用的物理页 : 1页目录表 + 1页表 + (256 - 2)页表 第一个页表 = 1页表,最后一个页表为页目录表 */
	for(uint32_t loop = 0;loop < 256;++loop) {
		bitmap_set(&kernel_physics_pool.pool_bitmap, loop, bitmap_used);
	}
}

/*
 @brief 申请1页物理内存
 @retval 物理页地址
 @note 申请了也不能直接用,必须映射到虚拟内存后使用虚拟内存访问
 */
void *malloc_physics_page(memory_pool_t *pool) {
	uint32_t bit_index = bitmap_continuous_scan(&pool->pool_bitmap, 1);
	if(bit_index == -1)
		return NULL;

	pool->pages_used++;
	pool->pages_free = pool->pages_total - pool->pages_used;

	bitmap_set(&pool->pool_bitmap, bit_index, bitmap_used);
	uint32_t physics_page = (bit_index << 12) + pool->addr_start;

	return (void *)physics_page;
}

/*
 @brief 释放1页物理内存
 @param p 要释放的物理页地址
 */
void free_physics_page(memory_pool_t *pool, void *physics_addr) {
	ASSERT((uint32_t)physics_addr > pool->addr_start || (uint32_t)physics_addr < pool->addr_end)

	uint32_t bit_index = ((uint32_t)physics_addr - pool->addr_start >> 12);

	bitmap_set(&pool->pool_bitmap, bit_index, bitmap_unused);
	pool->pages_used--;
	pool->pages_free = pool->pages_total - pool->pages_used;
}