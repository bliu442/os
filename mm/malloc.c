/*
 分配将一页内存以size为大小切割的内存块
 */

#include "../include/kernel/mm.h"
#include "../include/asm/system.h"
#include "../include/kernel/thread.h"
#include "../include/kernel/print.h"

#define TAG "malloc"
#define DEBUG_LEVEL 2
#include "../include/kernel/debug.h"

bucket_dir_t kernel_bucket_dir[10] = { //每一个内存块都有一条链表,链表中的每个节点有一页内存,从该节点中找可供分配的内存块
	{16, (bucket_desc_t *)0},
	{32, (bucket_desc_t *)0},
	{64, (bucket_desc_t *)0},
	{128, (bucket_desc_t *)0},
	{256, (bucket_desc_t *)0},
	{512, (bucket_desc_t *)0},
	{1024, (bucket_desc_t *)0},
	{2048, (bucket_desc_t *)0},
	{4096, (bucket_desc_t *)0},
	{0, (bucket_desc_t *)0},
};

/* this routine a linked list of free bucket descriptor blocks */
bucket_desc_t *free_bucket_desc = (bucket_desc_t *)0;

/*
 this routine initializes a bucket descriptor blocks
 @brief 申请一页内存,初始化为bucket descriptor链表
 */
static inline void init_bucket_desc(void) {
	bucket_desc_t *bdesc, *frist;
	int i;

	frist = bdesc = (bucket_desc_t *)malloc_kernel_page(1);
	if(!bdesc)
		return;

	for(i = PAGE_SIZE / sizeof(bucket_desc_t);i > 1;--i) {
		bdesc->next = bdesc + 1; //让空闲的bucket descriptor链起来
		++bdesc;
	}
	bdesc->next = free_bucket_desc;

	free_bucket_desc = frist;
}

void* kmalloc(size_t size) {
	task_t *current = running_thread();
	void * (*malloc_page_func)(uint32_t) = current->cr3 ? malloc_user_page : malloc_kernel_page;
	bucket_dir_t *bucket_dir = current->cr3 ? current->user_bucket_dir : kernel_bucket_dir;
	bucket_dir_t *bdir;
	bucket_desc_t *bdesc;
	void *retval;

	/*
	 first we search the bucket_dir to find the right bucket change for this request
	 找到内存大小合适的桶,在这个桶的bucket descriptor链表中的节点中找空闲内存块
	 */
	for(bdir = bucket_dir;bdir->size;++bdir) {
		if(bdir->size >= size)
			break;
	}
	if(!bdir->size) {
		ERROR("malloc called with impossibly large argument (%d)\r", size);
		return NULL;
	}

	CLI_FUNC

	/*
	 now we search for a bucket descriptor which has free space
	 找到合适的桶,再找一个有内存可以分配的bucket descriptor
	 */
	for(bdesc = bdir->chain;bdesc;bdesc = bdesc->next) {
		if(bdesc->freeptr)
			break;
	}

	/*
	 if we didn't find a bucket with free space, then we'll allocate a new one
	 如果没有找到空闲内存块,申请一页内存进行切割,分配
	 */
	if(!bdesc) {
		char *cp;
		int i;

		/* 是否有空闲的bucket descriptor,没有就创建一个新的bucket descriptor链表 */
		if(!free_bucket_desc)
			init_bucket_desc();

		bdesc = free_bucket_desc;
		free_bucket_desc = bdesc->next;

		/* 初始化bucket descriptor 申请内存页,切割内存,将bucket descriptor链接到bucket_dir里size下的链表中 */
		bdesc->refcnt = 0;
		bdesc->bucket_size = bdir->size;
		bdesc->page = bdesc->freeptr = (void *)(cp = (char *)malloc_page_func(1)); //申请内存页
		if(!cp)
			return NULL;
		
		/* set up the chain of free objects */
		for(i = PAGE_SIZE/bdir->size;i > 1;--i) { //*(char **)cp 修改指针指向地址里的数据
			*(char **)(cp) = cp + bdir->size; //切割内存 把下一个内存块地址写入上一个内存块的首四个字节中 freeptr用
			cp += bdir->size; //以bdir->size为大小切割的内存 内存块的首四个字节中的值可以看成一个链表
		}
		*((char **)cp) = 0; //最后一个写NULL
		
		bdesc->next = bdir->chain; //ok, link it in! 加入bdir->size下的链表中
		bdir->chain = bdesc; //更新链表头
	}

	retval = (void *)bdesc->freeptr; //拿到分配的内存块
	bdesc->freeptr = *(void **)retval; //拿到内存块首四字节的值(下一个要分配的地址)
	bdesc->refcnt++;

	INFO("kmalloc addr = %#x, size = %d\r", retval, bdir->size);
	STI_FUNC

	return retval;
}

void kfree(void *obj, size_t size) {
	task_t *current = running_thread();
	void (*free_page_func)(void *, uint32_t) = current->cr3 ? free_user_page : free_kernel_page;
	bucket_dir_t *bucket_dir = current->cr3 ? current->user_bucket_dir : kernel_bucket_dir;
	void *page;
	bucket_dir_t *bdir;
	bucket_desc_t *bdesc = NULL, *prev = NULL;

	/*
	 calculate what page this object lives in
	 计算obj所在的内存页地址
	 */
	page = (void *)((unsigned long)obj & 0xfffff000);

	/*
	 now search the buckets looking for that page
	 找要释放的桶,拿到相应的链表，对比bucket descriptor挂载的页地址
	 */
	for(bdir = bucket_dir;bdir->size;bdir++) {
		prev = 0;
		/* if size is zero then this conditional is always false */
		if(bdir->size < size)
			continue;
		for(bdesc = bdir->chain;bdesc;bdesc = bdesc->next) {
			if(bdesc->page == page)
				goto found;
			prev = bdesc;
		}
	}
	return;
found:
	CLI_FUNC

	*((void **)obj) = bdesc->freeptr; //把这块内存首四字节地址写bdesc->freeptr
	bdesc->freeptr = obj; //下次申请先把这块内存分配出去
	INFO("kfree addr = %#x, size = %d\r", obj, bdir->size);

	bdesc->refcnt--;
	if(bdesc->refcnt == 0) { //计数为0,这块内存页没有被使用的内存,可以释放
		/*
		 we need to make sure that prev is still accurate
		 it may not be, if someone rudely interrupted us...
		 */
		if((prev && (prev->next != bdesc)) || (!prev && (bdir->chain != bdesc))) {
			for(prev = bdir->chain;prev;prev = prev->next) {
				if(prev->next == bdesc) //确保
					break;
			}
		}

		if(prev)
			prev->next = bdesc->next; //bdesc不是第一个
		else { //bdesc是第一个
			if(bdir->chain != bdesc)
				return;
			bdir->chain = bdesc->next;
		}
		free_page_func(bdesc->page, 1);

		bdesc->next = free_bucket_desc; //把bdesc加入空闲的bucket descriptor链表
		free_bucket_desc = bdesc; //先分配这个bucket descriptor
	}

	STI_FUNC
	
	return;
}

