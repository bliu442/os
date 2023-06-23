/*
 pcb fd_table数组 数组下标:文件描述符 数组元素:file_table数组下标
 file_table数组元素:文件结构
 文件结构:记录文件操作相关信息 每打开一次文件产生一个文件结构
 */
#include "../include/kernel/file.h"
#include "../include/kernel/thread.h"
#include "../include/kernel/dir.h"
#include "../include/kernel/mm.h"
#include "../include/string.h"
#include "../include/kernel/inode.h"

#define TAG "file"
#define DEBUG_LEVEL 4
#include "../include/kernel/debug.h"

file_t file_table[MAX_FILE_OPEN]; // 文件表

/*
 @brief 在文件表中找到空闲
 @retval 文件表数组下标
 */
int32_t get_free_slot_in_file_table(void) {
	uint32_t fd_index = 3;
	while(fd_index < MAX_FILE_OPEN) {
		if(file_table[fd_index].fd_inode == NULL)
			break;
		fd_index++;
	}

	if(fd_index == MAX_FILE_OPEN) {
		WARN("execed max open file\r");
		return -1;
	}

	return fd_index;
}

/*
 @brief 在进程pcb文件描述符数组中找空闲,并将文件表数组下标写入
 @param global_fd_index 文件表数组下标
 @retval 返回文件描述符数组下标 失败-1
 */
int32_t pcb_fd_install(int32_t global_fd_index) {
	task_t *current = running_thread();

	uint8_t local_fd_index = 3;
	while(local_fd_index < MAX_FILES_OPEN_PRE_PROCESS) {
		if(current->fd_table[local_fd_index] == -1) {
			current->fd_table[local_fd_index] = global_fd_index;
			break;
		}
		local_fd_index++;
	}

	if(local_fd_index == MAX_FILES_OPEN_PRE_PROCESS) {
		WARN("execed max open file pre process\r");
		return -1;
	}

	return local_fd_index;
}

/*
 @brief 在父目录下新建文件
 @param parent_dir 父目录
 @param filename 文件名
 @param flag
 @retval 文件描述符 -1:出错
 */
int32_t file_create(dir_t *parent_dir, char *filename, uint8_t flag) {
	void *buf = kmalloc(1024);
	if(buf == NULL) {
		WARN("kmalloc\r");
		return -1;
	}

	uint8_t roolback_step = 0;

	int32_t inode_no = inode_bitmap_alloc(current_part);
	if(inode_no == -1) {
		WARN("allocate inode failed\r");
		return -1;
	}

	inode_t *new_inode = (inode_t *)kmalloc(sizeof(inode_t));
	if(new_inode == NULL) {
		WARN("kmalloc\r");
		roolback_step = 1;
		goto roolback;
	}
	inode_init(inode_no, new_inode);

	int fd_index = get_free_slot_in_file_table();
	if(fd_index == -1) {
		WARN("execed max open file\r");
		roolback_step = 2;
		goto roolback;
	}

	file_table[fd_index].fd_pos = 0;
	file_table[fd_index].fd_flag = flag;
	file_table[fd_index].fd_inode = new_inode;

	dir_entry_t new_dir_entry = {0};
	dir_entry_create(filename, inode_no, FILE_REGULAR, &new_dir_entry);

	if(!dir_entry_sync(parent_dir, &new_dir_entry, buf)) {
		WARN("sync dir entry to disk failed\r");
		roolback_step = 3;
		goto roolback;
	}

	memset(buf, 0, 1024);
	inode_sync(current_part, parent_dir->inode, buf);

	memset(buf, 0, 1024);
	inode_sync(current_part, new_inode, buf);

	bitmap_sync(current_part, inode_no, BITMAP_INODE);

	list_push(&current_part->open_inodes, &new_inode->i_list_item);
	new_inode->i_open_counts = 1;

	kfree(buf, 1024);
	return pcb_fd_install(fd_index);

roolback:
	switch(roolback_step) {
		case 3:
			memset(&file_table[fd_index], 0, sizeof(file_t));
		case 2:
			kfree(new_inode, sizeof(inode_t));
		case 1:
			bitmap_set(&current_part, inode_no, bitmap_unused);
			break;
	}

	kfree(buf, 1024);
	return -1;
}