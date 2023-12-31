#ifndef __MM_H__
#define __MM_H__

#include "../stdint.h"
#include "./bitmap.h"
#include "./sync.h"

#define PAGE_SIZE 4096

/* 页目录项,页表项属性 */
#define PAGE_P (1)
#define PAGE_RW_R (0 << 1)
#define PAGE_RW_W (1 << 1)
#define PAGE_US_S (0 << 2)
#define PAGE_US_U (1 << 2)

/* 内存分布 */
#define ARDS_ADDR 0x1002 //bios中断检测内存
#define ARDS_TIMES_ADDR 0x1000

#define OS_MEMORY_END 0x100000 //1M以下存放os代码
#define OS_PAGE_DIR_TABLE_POS 0x100000 //setup os页目录表
#define KERNEL_PHYSICS_MEMORY_MAP 0xC0090000 //放到堆栈下面 堆栈0x9F000 + 0xC0000000 堆栈处于1M内存最高处

#define KERNEL_VIRTUAL_MEMORY_START 0xC0100000 //放在os代码后面

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
	lock_t pool_lock;
	uint32_t addr_start; //可用内存起始地址
	uint32_t valid_memory_size; //可用内存大小
	uint32_t addr_end; //可用内存结束地址
	uint32_t pages_total; //一共分多少页
	uint32_t pages_free; //空闲页数量
	uint32_t pages_used; //使用页数量
}memory_pool_t;
extern memory_pool_t kernel_physics_pool;
extern memory_pool_t user_physics_pool;
extern memory_pool_t kernel_virtual_pool;

/* malloc内存管理 */
typedef struct bucket_desc {
	void *page; //管理的内存页地址,释放的时候用
	struct bucket_desc *next; //下一个bucket descriptor地址 构成链表
	void *freeptr; //可供分配的内存块地址
	unsigned short refcnt; //计数,释放内存页时用
	unsigned short bucket_size; //桶内存块大小
}bucket_desc_t;

typedef struct bucket_dir { //分配内存方案 内存块大小+桶描述符链表
	int size;
	bucket_desc_t *chain;
}bucket_dir_t;

extern void print_check_memory_info(void);

extern void physics_memory_init(void);
extern void physics_memory_pool_init(void);
extern void virtual_memory_init(void);
extern void virtual_memory_pool_init(void);

/* 提供给malloc_page使用 */
extern void *malloc_physics_page(memory_pool_t *pool);
extern void free_physics_page(memory_pool_t *pool, void *physics_addr);

extern uint32_t addr_virtual2physics(uint32_t virtual_addr);
extern uint32_t *get_pde_ptr(uint32_t virtual_addr);
extern uint32_t *get_pte_ptr(uint32_t virtual_addr);
extern void page_table_add(uint32_t virtual_addr,uint32_t physics_addr);
extern void page_table_remove(pool_flag_t pf, uint32_t virtual_addr);

extern void *malloc_page(pool_flag_t pf, uint32_t count);
extern void free_page(pool_flag_t pf, void *virtual_addr_start, uint32_t count);
extern void *malloc_a_page(pool_flag_t pf, uint32_t virtual_addr);
extern void *malloc_a_page_without_add_virtual_bitmap(pool_flag_t pf, uint32_t virtual_addr);
extern void *malloc_kernel_page(uint32_t count);
extern void free_kernel_page(void *virtual_addr, uint32_t count);
extern void *malloc_user_page(uint32_t count);
extern void free_user_page(void *virtual_addr, uint32_t count);

extern void* kmalloc(size_t size);
extern void kfree(void *obj, size_t size);

#endif