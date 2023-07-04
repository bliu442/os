/*
 所有打开的节点内存都使用的是内核空间的内存,以便所有进程都能直接访问文件
 打开的节点使用链表维护
 */

#include "../include/kernel/inode.h"
#include "../include/kernel/thread.h"
#include "../include/asm/system.h"
#include "../include/kernel/hd.h"
#include "../include/kernel/fs.h"

#define TAG "inode"
#define DEBUG_LEVEL 4
#include "../include/kernel/debug.h"

typedef struct inode_position {
	bool two_sector; //占两个扇区
	uint32_t sector_lba; //起始扇区地址
	uint32_t offset; //扇区内偏移
}inode_position_t;

/*
 @brief 初始化inode
 @param inode_no inode索引
 @param new_inode inode地址
 */
void inode_init(uint32_t inode_no, inode_t *new_inode) {
	new_inode->i_no = inode_no;
	new_inode->i_mode = 0;
	new_inode->i_size = 0;
	new_inode->i_time = 0;
	new_inode->i_open_counts = 0;
	new_inode->i_write_deny = false;
	
	memset(new_inode->i_zone, 0, sizeof(new_inode->i_zone));
}

/*
 @brief 获取inode在硬盘中的位置
 @param inode_no 想要获取的inode inode_no
 @param inode_pos 存放inode位置
 */
static void inode_locate(hd_partition_t *part, uint32_t inode_no, inode_position_t *indoe_pos) { 
	uint32_t inode_table_lba = part->sb->inode_table_lab;

	uint32_t inode_size = sizeof(inode_t);
	uint32_t offset_byte = inode_no * inode_size;
	uint32_t offset_sector = offset_byte / SECTOR_SIZE;
	uint32_t offset_byte_in_sector = offset_byte % SECTOR_SIZE;

	uint32_t left_in_sector = SECTOR_SIZE - offset_byte_in_sector; //如果inode跨扇区
	if(left_in_sector < inode_size)
		indoe_pos->two_sector = true;
	else
		indoe_pos->two_sector = false;

	indoe_pos->sector_lba = inode_table_lba + offset_sector;
	indoe_pos->offset = offset_byte_in_sector;
}

/*
 @brief 将inode同步到硬盘
 @param inode inode地址
 @buf 缓冲区
 */
void inode_sync(hd_partition_t *part, inode_t *inode, void *buf) {
	uint8_t inode_no = inode->i_no;
	inode_position_t inode_pos;
	inode_locate(part, inode_no, &inode_pos);

	inode_t pure_inode;
	memcpy(&pure_inode, inode, sizeof(inode_t));

	pure_inode.i_open_counts = 0; //不需要保存的参数
	pure_inode.i_write_deny = false;
	pure_inode.i_list_item.prev = pure_inode.i_list_item.next = NULL;

	char *inode_buf = (char *)buf;
	uint8_t count = inode_pos.two_sector ? 2 : 1;
	hd_read(part->disk, inode_pos.sector_lba, count, inode_buf);
	memcpy((inode_buf + inode_pos.offset), &pure_inode, sizeof(inode_t));
	hd_write(part->disk, inode_pos.sector_lba, count, inode_buf);
}

/*
 @brief 释放指定inode号对应的inode结构和相关的数据块
 @param inode_no 要释放的节点号
 @note
 1.block位图回收
 2.inode位图回收
 只同步位图,实际数据并未删除,新数据覆盖
 */
void inode_release(hd_partition_t *part, uint32_t inode_no) {
	inode_t *delete_inode = inode_open(part, inode_no);

	uint8_t block_index = 0;
	uint8_t block_number = 12;
	uint32_t all_block[140] = {0};
	uint32_t block_bitmap_index = 0;

	while(block_index < block_number) {
		all_block[block_index] = delete_inode->i_zone[block_index];
		block_index++;
	}
	if(delete_inode->i_zone[block_index] != 0) {
		hd_read(part, delete_inode->i_zone[block_index], 1, &all_block[block_index]);
		block_number = 140;
		
		block_bitmap_index = delete_inode->i_zone[block_index] - part->sb->data_start_lba;
		bitmap_set(&part->block_bitmap, block_bitmap_index, bitmap_unused);
		bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);
	}

	block_index = 0;
	while(block_index < block_number) {
		if(all_block[block_index] != 0) {
			block_bitmap_index = all_block[block_index] - part->sb->data_start_lba;
			bitmap_set(&part->block_bitmap, block_bitmap_index, bitmap_unused);
			bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);
		}

		block_index++;
	}

	bitmap_set(&part->inode_bitmap, inode_no, bitmap_unused);
	bitmap_sync(part, inode_no, BITMAP_INODE);

	inode_close(delete_inode);
}

/*
 @brief 打开一个指定的inode节点,以便进行后续的文件操作
 @retval inode节点的指针
 @note 申请内存存放inode,并将inode加入链表 内存使用内核空间内存,所有进程都能访问
 */
inode_t *inode_open(hd_partition_t *part, uint32_t inode_no) {
	list_item_t *item = part->open_inodes.head.next;
	inode_t *inode_found;
	while(item != &part->open_inodes.tail) { //查看该文件是否已经打开
		inode_found = item2entry(inode_t, i_list_item, item);
		if(inode_found->i_no == inode_no) {
			inode_found->i_open_counts++;
			return inode_found;
		}

		item = item->next;
	}

	/* 没打开过,从硬盘加载 */
	inode_position_t inode_pos;
	inode_locate(part, inode_no, &inode_pos);

	task_t *current = running_thread();
	uint32_t cr3_bak = current->cr3;
	current->cr3 = NULL;
	inode_found = kmalloc(sizeof(inode_t));
	if(inode_found == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	current->cr3 = cr3_bak;

	char *inode_buf;
	uint8_t count = inode_pos.two_sector ? 2 : 1;
	inode_buf = kmalloc(SECTOR_SIZE * count);
	if(inode_buf == NULL) {
		PANIC("kmalloc\r");
		kfree(inode_found, sizeof(inode_t));
		return;
	}
	hd_read(part->disk, inode_pos.sector_lba, count, inode_buf);
	memcpy(inode_found, inode_buf + inode_pos.offset, sizeof(inode_t));

	list_push(&part->open_inodes, &inode_found->i_list_item);
	inode_found->i_open_counts = 1;

	kfree(inode_buf, SECTOR_SIZE * count);
	return inode_found;
}

/*
 @brief 关闭相应的inode
 @note 释放内存,清除链表中的节点
 */
void inode_close(inode_t *inode) {
	CLI_FUNC

	if(--inode->i_open_counts == 0) {
		list_remove(&inode->i_list_item);

		task_t *current = running_thread();
		uint32_t cr3_bak = current->cr3;
		current->cr3 = NULL;
		kfree(inode, sizeof(inode_t));
		current->cr3 = cr3_bak;
	}

	STI_FUNC
}


