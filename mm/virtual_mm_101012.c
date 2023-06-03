/*
 虚拟内存管理 101012
 cr3 存储 页目录表物理地址
 页目录表(pdt) 存储 页目录表项(pde)物理地址
 页目录表项 = 页表(ptt)
 页表 存储 页表项(pte)物理地址
 页表项 存储 虚拟地址映射的物理地址

 cr3 pde pte存储的都是物理地址,给cpu访问

 pte
 1.存储的是映射的物理页地址
 2.本身代表虚拟地址,根据索引来计算
 eg:pte是第1025个,这个pte代表的虚拟页地址是1025 * PAGE_SIZE 这个pte是第二个ptt的第一个

 1个ptt可以存放PAGE_SIZE / 4 个pte,映射4M内存
 1个pdt可以存放PAGE_SIZE / 4 个pde,映射4G内存
 */

#include "../include/kernel/mm.h"
#include "../include/kernel/print.h"
#include "../include/kernel/debug.h"
#include "../include/string.h"
#include "../include/kernel/thread.h"
#include "../include/asm/system.h"

#define PDE_INDEX(virtual_addr) (virtual_addr >> 22)
#define PTE_INDEX(virtual_addr) (virtual_addr >> 12 & 0x3FF)

memory_pool_t kernel_virtual_pool;

/* @brief 虚拟内存管理方案 */
void virtual_memory_init(void) {
	kernel_virtual_pool.addr_start = KERNEL_VIRTUAL_MEMORY_START;
	kernel_virtual_pool.valid_memory_size = kernel_physics_pool.valid_memory_size - 256 * PAGE_SIZE;
	kernel_virtual_pool.addr_end = kernel_virtual_pool.addr_start + kernel_virtual_pool.valid_memory_size;
	kernel_virtual_pool.pages_total = kernel_virtual_pool.valid_memory_size / PAGE_SIZE;
	kernel_virtual_pool.pages_used = 0;
	kernel_virtual_pool.pages_free = kernel_virtual_pool.pages_total - kernel_virtual_pool.pages_used;
	
	put_str("kernel virtual memory position :");
	put_int(kernel_virtual_pool.addr_start);
	put_char('-');
	put_int(kernel_virtual_pool.addr_end);
	put_str("    used : ");
	put_int(kernel_virtual_pool.pages_used);
	put_str("page\r");
}

/* @brief 实现虚拟内存管理方案 bitmap使用1位表示1页虚拟内存是否被使用 */
void virtual_memory_pool_init(void) {
	ASSERT(kernel_virtual_pool.valid_memory_size != 0);

	kernel_virtual_pool.pool_bitmap.bitmap_byte_len = kernel_virtual_pool.pages_total / 8;
	// if(kernel_virtual_pool.pages_total % 8)
	// 	kernel_virtual_pool.pool_bitmap.bitmap_byte_len++;

	kernel_virtual_pool.pool_bitmap.map = (uint8_t *)(KERNEL_PHYSICS_MEMORY_MAP \
	+ kernel_physics_pool.pool_bitmap.bitmap_byte_len \
	+ user_physics_pool.pool_bitmap.bitmap_byte_len);

	bitmap_init(&kernel_virtual_pool.pool_bitmap);

	put_str("kernel virtual memory pool position: ");
	put_int((uint32_t)kernel_virtual_pool.pool_bitmap.map);
	put_char('-');
	put_int((uint32_t)kernel_virtual_pool.pool_bitmap.map + kernel_virtual_pool.pool_bitmap.bitmap_byte_len);
	put_char('\r');
}

/*
 @brief 按页申请虚拟内存
 @param pf 申请物理页的类型
 @param count 申请多少页
 @retval void * 返回虚拟页地址
 */
static void *malloc_virtual_page(pool_flag_t pf, uint32_t count) {
	uint32_t virtual_addr = 0;
	uint32_t bit_index = -1;
	uint32_t find_count = 0;
	memory_pool_t *pool = NULL;

	if(pf == pf_kernel) {
		pool = &kernel_virtual_pool;
	} else if(pf == pf_user) {
		task_t *current = running_thread();
		pool = &current->user_virtual_pool;
	}

	bit_index = bitmap_continuous_scan(&pool->pool_bitmap, count);
	if(bit_index == -1)
		return NULL;
	while(find_count < count) {
		bitmap_set(&pool->pool_bitmap, bit_index + find_count++, bitmap_used);
		pool->pages_used++;
	}
	virtual_addr = pool->addr_start + bit_index * PAGE_SIZE;

	pool->pages_free = pool->pages_total - pool->pages_used;

	return (void *)virtual_addr;
}

/*
 @brief 按页释放虚拟内存
 @param pf 释放物理页的类型
 @param virtual_addr 要释放的虚拟页地址
 @param count 释放多少页
 */
static void free_virtual_page(pool_flag_t pf, void *virtual_addr, uint32_t count) {
	memory_pool_t *pool = NULL;
	uint32_t free_count = 0;

	if(pf == pf_kernel) {
		pool = &kernel_virtual_pool;
	} else if(pf == pf_user) {
		task_t *current = running_thread();
		pool = &current->user_virtual_pool;
	}

	ASSERT((uint32_t)virtual_addr > pool->addr_start || (uint32_t)virtual_addr < pool->addr_end);

	uint32_t bit_index = (((uint32_t)virtual_addr - pool->addr_start) / PAGE_SIZE);

	while(free_count < count) {
		bitmap_set(&pool->pool_bitmap, bit_index + free_count++, bitmap_unused);
		pool->pages_used--;
	}

	pool->pages_free = pool->pages_total - pool->pages_used;
}

/*
 @brief 获取虚拟地址对应的物理地址
 @param virtual_addr 虚拟地址
 @retval 物理地址
 */
uint32_t addr_virtual2physics(uint32_t virtual_addr) {
	uint32_t *pte = get_pte_ptr(virtual_addr);
	return ((*pte & 0xFFFFF000) + (virtual_addr & 0xFFF));
}

/*
 @brief 构造虚拟地址访问virtual_addr的pde
 @param virtual_addr 虚拟地址
 @retval virtual_addr的pde指针 指针是个虚拟地址,程序中可以使用
 @note virtual_addr的pde地址 = cr3 + pde_index * 4,cr3:pdt页基址 pde_index * 4:页内偏移地址

 构造地址找pde -> pde存放在pdt中 -> 构造的虚拟地址映射的物理地址得是pdt所在的物理页
 1.cpu访问到pdt cr3
 2.cpu访问到pde/ptt 0x3FF << 22,pdt的1023存储的是pdt本身,将pdt当作ptt访问
 3.cpu访问到pte 0x3FF << 12,pdt的1023存储的是pdt本身,将pdt当作物理页访问
 4.cpu拿到物理页地址 实现将pdt当作物理页访问 再提供页内偏移地址修改pde
 5.构造的虚拟地址 = (0x3FF << 22) + (0x3FF << 12) + virtual_addr的pde的页内偏移地址
 */
uint32_t *get_pde_ptr(uint32_t virtual_addr) {
	uint32_t *pde = (uint32_t *)(0xFFFFF000 + PDE_INDEX(virtual_addr) * 4);
	return pde;
}

/*
 @brief 构造虚拟地址访问virtual_addr的pte
 @param virtual_addr 虚拟地址
 @retval virtual_addr的pte指针 指针是个虚拟地址,程序中可以使用
 @note virtual_addr的pte地址 = pde里存的值 + pte_index * 4,pde里存的值:ptt页基址 pte_index * 4:页内偏移地址

 构造地址找pte -> pte存放在ptt中 -> 构造的虚拟地址映射的物理地址得是ptt所在的物理页
 1.cpu访问到pdt cr3
 2.cpu访问到pde/ptt 0x3FF << 22,pdt的1023存储的是pdt本身,将pdt当作ptt访问
 3.cpu访问到pte pde_index,pdt的pde_inde存储的是pde/ptt,将ptt当作物理页访问
 4.cpu拿到物理页地址 实现将ptt当作物理页访问 再提供页内偏移地址修改pte
 5.构造的地址 = (0x3FF << 22) + (pde_index << 12) + virtual_addr的pte的页内偏移地址
 */
uint32_t *get_pte_ptr(uint32_t virtual_addr) {
	uint32_t *pte = (uint32_t *)(0xFFC00000 + ((PDE_INDEX(virtual_addr) << 12) + PTE_INDEX(virtual_addr) * 4));
	return pte;
}

/*
 @brief 将物理地址映射到虚拟地址
 @param virtual_addr 虚拟地址
 @param physics_addr 物理地址
 */
void page_table_add(uint32_t virtual_addr,uint32_t physics_addr) {
	uint32_t *pde = get_pde_ptr(virtual_addr);
	uint32_t *pte = get_pte_ptr(virtual_addr);

	if(*pde & 0x00000001) { //p位 页表存在
		*pte = (physics_addr | 0x7);
	} else { //页表不存在
		uint32_t *ptt = (uint32_t *)malloc_physics_page(&kernel_physics_pool);
		*pde = (uint32_t)ptt | 0x7; //填充1个pde

		/* pte是ptt的子项,将低12位清理可访问到ptt */
		memset((void *)((uint32_t)pte & 0xFFFFF000), 0, PAGE_SIZE); //清零,防止页表混乱

		// BOCHS_DEBUG_MAGIC
		// BOCHS_DEBUG_MAGIC

		*pte = (physics_addr | 0x7);
	}
}

/*
 @brief 删除虚拟地址映射的物理地址
 @param virtual_addr 虚拟地址
 */
void page_table_remove(pool_flag_t pf, uint32_t virtual_addr) {
	memory_pool_t *pool = pf & pf_user ? &user_physics_pool : &kernel_physics_pool;

	uint32_t *pde = get_pde_ptr(virtual_addr);
	uint32_t *pte = get_pte_ptr(virtual_addr);

	if(*pde & 0x00000001) {
		uint32_t physics_addr = (*pte & 0xFFFFF000);
		free_physics_page(pool, (void *)physics_addr);
		*pte = 0;
	}

	//mark 页表回收
}

/*
 @brief 申请内存
 @param pf 内核/用户物理内存池
 @param count 申请多少页
 @retval 虚拟地址

 1.虚拟内存池中申请虚拟内存
 2.物理内存池中申请物理内存
 3.在页表中完成映射
 */
void *malloc_page(pool_flag_t pf, uint32_t count) {
	void *virtual_addr_start = malloc_virtual_page(pf, count);
	if(virtual_addr_start == NULL)
		return NULL;

	memory_pool_t *pool = pf & pf_user ? &user_physics_pool : &kernel_physics_pool;
	uint32_t find_count = 0;
	uint32_t virtual_addr = (uint32_t)virtual_addr_start;
	while(find_count < count) {
		void *physics_addr = malloc_physics_page(pool);
		if(physics_addr == NULL)
			return NULL;
		
		page_table_add(virtual_addr, (uint32_t)physics_addr);
		virtual_addr += PAGE_SIZE;
		find_count++;
	}
	
	return virtual_addr_start;
}

/*
 @brief 释放内存
 @param pf 内核/用户物理内存池
 @param virtual_addr_start 虚拟内存地址
 @param count 释放多少页

 1.释放虚拟内存
 2.释放物理内存
 3.页表中解除映射
 */
void free_page(pool_flag_t pf, void *virtual_addr_start, uint32_t count) {
	free_virtual_page(pf, virtual_addr_start, count);

	uint32_t free_count = 0;
	uint32_t virtual_addr = (uint32_t)virtual_addr_start;
	while(free_count < count)
		page_table_remove(pf, virtual_addr + free_count++ * PAGE_SIZE);
}

/*
 @brief 申请一页物理内存,并将virtual_addr映射到该地址
 @param pf 内核/用户物理内存池
 @param virtual_addr 指定的虚拟地址
 @retval 虚拟地址
 @note 可以指定虚拟地址,申请一页物理页映射到该地址处
 malloc_kernel/user_page 是内存管理模块分配的地址
 
 1.设置虚拟内存池相应位为到bitmap_used
 2.物理内存池中申请物理内存
 3.在页表中完成映射
 */
void *malloc_a_page(pool_flag_t pf, uint32_t virtual_addr) {
	memory_pool_t *physics_pool = pf & pf_user ? &user_physics_pool : &kernel_physics_pool;
	lock_acquire(&physics_pool->pool_lock);

	task_t *current = running_thread();
	uint32_t bit_index = -1;
	memory_pool_t *virtual_pool = NULL;

	if(current->cr3 != 0 && pf == pf_user) { //用户进程
		virtual_pool = &current->user_virtual_pool;
	} else if(current->cr3 == 0 && pf == pf_kernel) { //内核线程
		virtual_pool = &kernel_virtual_pool;
	} else {
		PANIC("malloc a page!\r");
	}

	bit_index = (virtual_addr - virtual_pool->addr_start) / PAGE_SIZE;
	ASSERT(bit_index > 0);
	ASSERT(bitmap_used != bitmap_scan(&virtual_pool->pool_bitmap, bit_index));
	bitmap_set(&virtual_pool->pool_bitmap, bit_index, bitmap_used);

	void *physics_addr = malloc_physics_page(physics_pool);
	if(physics_addr == NULL) {
		lock_release(&physics_pool->pool_lock);
		INFO("malloc_physics_page\r");
		return NULL;
	}

	page_table_add(virtual_addr, (uint32_t)physics_addr);

	lock_release(&physics_pool->pool_lock);
	return (void *)virtual_addr;
}

/*
 @brief 申请一页内存,不添加到虚拟内存池管理中
 @param pf 内核/用户物理内存池
 @param virtual_addr 指定的虚拟地址
 @retval 虚拟地址
 @note 可以指定虚拟地址,申请一页物理页映射到该地址处
 @note 仅限fork函数使用, 子进程pcb是拷贝父进程,bitmap已经置位
 */
void *malloc_a_page_without_add_virtual_bitmap(pool_flag_t pf, uint32_t virtual_addr) {
	memory_pool_t *physics_pool = pf & pf_user ? &user_physics_pool : &kernel_physics_pool;
	lock_acquire(&physics_pool->pool_lock);
	void *physics_addr = malloc_physics_page(physics_pool);
	if(physics_addr == NULL) {
		lock_release(&physics_pool->pool_lock);
		INFO("malloc_physics_page\r");
		return NULL;
	}

	page_table_add(virtual_addr, (uint32_t)physics_addr); //子进程虚拟地址不变,实际使用物理地址与父进程不同
	lock_release(&physics_pool->pool_lock);

	return (void *)virtual_addr;
}

void *malloc_kernel_page(uint32_t count) {
	lock_acquire(&kernel_physics_pool.pool_lock);
	void *virtual_addr = malloc_page(pf_kernel, count);
	if(virtual_addr != NULL)
		memset(virtual_addr, 0, count * PAGE_SIZE);
	
	lock_release(&kernel_physics_pool.pool_lock);
	return virtual_addr;
}

void free_kernel_page(void *virtual_addr, uint32_t count) {
	lock_acquire(&kernel_physics_pool.pool_lock);
	free_page(pf_kernel, virtual_addr, count);
	lock_release(&kernel_physics_pool.pool_lock);
}

void *malloc_user_page(uint32_t count) {
	lock_acquire(&user_physics_pool.pool_lock);
	void *virtual_addr = malloc_page(pf_user, count);
	if(virtual_addr != NULL)
		memset(virtual_addr, 0, count * PAGE_SIZE);
	
	lock_release(&user_physics_pool.pool_lock);
	return virtual_addr;
}

void free_user_page(void *virtual_addr, uint32_t count) {
	lock_acquire(&user_physics_pool.pool_lock);
	free_page(pf_user, virtual_addr, count);
	lock_release(&user_physics_pool.pool_lock);
}