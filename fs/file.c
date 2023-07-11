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
#include "../include/asm/system.h"
#include "../include/kernel/hd.h"
#include "../include/kernel/fs.h"

#define TAG "file"
#define DEBUG_LEVEL 4
#include "../include/kernel/debug.h"

file_t file_table[MAX_FILE_OPEN]; // 文件表

uint32_t fd_local2global(uint32_t local_fd) {
	task_t *current = running_thread();
	uint32_t global_fd = current->fd_table[local_fd];
	return global_fd;
}
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
 @retval 成功:inode_no -1:出错
 @note 写了两次硬盘
 1.父目录inode节点更新
 2.新目录inode节点写入
 */
int32_t file_create(dir_t *parent_dir, char *filename) {
	void *buf = kmalloc(1024);
	if(buf == NULL) {
		WARN("kmalloc\r");
		return -1;
	}

	uint8_t rollback_step = 0;

	int32_t inode_no = inode_bitmap_alloc(current_part);
	if(inode_no == -1) {
		WARN("allocate inode failed\r");
		rollback_step = 1;
	}

	inode_t new_inode = {0};
	inode_init(inode_no, &new_inode);

	dir_entry_t new_dir_entry = {0};
	dir_entry_create(filename, inode_no, FILE_REGULAR, &new_dir_entry);

	if(!dir_entry_sync(parent_dir, &new_dir_entry, buf)) {
		WARN("sync dir entry to disk failed\r");
		rollback_step = 2;
		goto rollback;
	}

	memset(buf, 0, 1024);
	inode_sync(current_part, parent_dir->inode, buf);

	memset(buf, 0, 1024);
	inode_sync(current_part, &new_inode, buf);

	bitmap_sync(current_part, inode_no, BITMAP_INODE);

	kfree(buf, 1024);
	return inode_no;

rollback:
	switch(rollback_step) {
		case 2:
			bitmap_set(&current_part, inode_no, bitmap_unused);
		case 1:
			kfree(buf, 1024);
			break;
	}

	return -1;
}

/*
 @brief 打开指定inode号对应的文件,并将其加入进程控制块的文件描述符表中
 @retval 文件描述符
 */
int32_t file_open(uint32_t inode_no, uint8_t flag) {
	int fd_index = get_free_slot_in_file_table();
	if(fd_index == -1) {
		WARN("execed max open file\r");
		return -1;
	}
	file_table[fd_index].fd_inode = inode_open(current_part, inode_no);
	file_table[fd_index].fd_flag = flag;
	file_table[fd_index].fd_pos = 0;

	bool *write_deny = &file_table[fd_index].fd_inode->i_write_deny;

	if(flag == O_WRONLY || flag == O_RDWR) {
		CLI_FUNC

		if(!(*write_deny)) {
			*write_deny = true;
			STI_FUNC
		} else {
			WARN("file can't be write now, try again later\r");
			STI_FUNC
			return -1;
		}
	}

	return pcb_fd_install(fd_index);
}

int32_t file_close(file_t *file) {
	if(file == NULL) {
		return -1;
	}

	if(file->fd_flag == O_WRONLY || file->fd_flag == O_RDWR)
		file->fd_inode->i_write_deny = false;
	inode_close(file->fd_inode);
	file->fd_inode = NULL; // 释放文件表元素
	return 0;
}

int32_t file_write(file_t *file, const void *buf, uint32_t count) {
	uint32_t rollback_step = 0;
	if(file->fd_inode->i_size + count > BLOCK_SIZE * 140) {
		ERROR("exceed max file size %d bytes, write file failed\r", BLOCK_SIZE * 140);
		return -1;
	}
	uint8_t *io_buf = kmalloc(BLOCK_SIZE);
	if(io_buf == NULL) {
		ERROR("kmalloc\r");
		return -1;
	}
	uint32_t *all_block = kmalloc(140 * 4);
	if(all_block == NULL) {
		ERROR("kmalloc\r");
		rollback_step = 1;
		goto rollback;
	}

	const uint8_t * src = buf; // 待写入数据
	uint32_t bytes_writeen = 0; // 统计写入多少byte
	uint32_t size_left = count; // 统计未写入的数据

	uint32_t block_index = 0;
	uint32_t block_lba = 0;
	uint32_t block_bitmap_index = 0;
	uint32_t sector_lba = 0;
	uint32_t sector_index = 0;
	uint32_t sector_offset_bytes = 0;
	uint32_t sector_left_bytes = 0;
	uint32_t chunk_size = 0;
	uint32_t indirect_block_table = 0;

	if(file->fd_inode->i_zone[0] == 0) { // 第一次写
		block_lba = block_bitmap_alloc(current_part);
		if(block_lba == -1) {
			ERROR("block bitmap alloc failed\r");
			rollback_step = 2;
			goto rollback;
		}
		file->fd_inode->i_zone[0] = block_lba;
		block_bitmap_index = block_lba - current_part->sb->data_start_lba;
		bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK); // 不需要回滚
	}

	uint32_t file_has_used_blocks = file->fd_inode->i_size ? DIV_ROUND_UP(file->fd_inode->i_size, BLOCK_SIZE) : 1;
	uint32_t file_will_use_blocks = DIV_ROUND_UP(file->fd_inode->i_size + count, BLOCK_SIZE);
	ASSERT(file_will_use_blocks < 140);

	uint32_t add_blocks = file_will_use_blocks - file_has_used_blocks;
	/* 将需要改动的block收集到all_block */
	if(add_blocks == 0) {
		if(file_has_used_blocks <= 12) {
			block_index = file_has_used_blocks -1;
			all_block[block_index] = file->fd_inode->i_zone[block_index];
		} else {
			indirect_block_table = file->fd_inode->i_zone[12];
			hd_read(current_part->disk, indirect_block_table, 1, all_block + 12);
		}
	} else { // 需要分配block
		if(file_will_use_blocks <= 12) {
			block_index = file_has_used_blocks -1;
			all_block[block_index] = file->fd_inode->i_zone[block_index];

			block_index = file_has_used_blocks;
			while(block_index < file_will_use_blocks) {
				ASSERT(file->fd_inode->i_zone[block_index] == 0);

				block_lba = block_bitmap_alloc(current_part);
				if(block_lba == -1) {
					ERROR("block bitmap alloc failed\r");
					rollback_step = 3;
					goto rollback;
				}
				file->fd_inode->i_zone[block_index] = block_lba;
				block_bitmap_index = block_lba - current_part->sb->data_start_lba;
				bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);
				block_index++;
			}
		} else if(file_has_used_blocks <= 12) { // 需要分配一级间接表
			ASSERT(file->fd_inode->i_zone[12] == 0);
			block_lba = block_bitmap_alloc(current_part);
			if(block_lba == -1) {
				ERROR("block bitmap alloc failed\r");
				rollback_step = 2;
				goto rollback;
			}
			file->fd_inode->i_zone[12] = block_lba;
			block_bitmap_index = block_lba - current_part->sb->data_start_lba;
			bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);

			block_index = file_has_used_blocks - 1;
			all_block[block_index] = file->fd_inode->i_zone[block_index];

			block_index = file_has_used_blocks;
			while(block_index < file_will_use_blocks) {
				ASSERT(file->fd_inode->i_zone[block_index] == 0);

				block_lba = block_bitmap_alloc(current_part);
				if(block_lba == -1) {
					ERROR("block bitmap alloc failed\r");
					rollback_step = 3;
					goto rollback;
				}

				if(block_index < 12) {
					file->fd_inode->i_zone[block_index] = all_block[block_index] = block_lba;
				} else {
					all_block[block_index] = block_lba;
				}

				block_bitmap_index = block_lba - current_part->sb->data_start_lba;
				bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);
				block_index++;
			}
		} else {
			ASSERT(file->fd_inode->i_zone[12] != 0);

			indirect_block_table = file->fd_inode->i_zone[12];
			hd_read(current_part->disk, indirect_block_table, 1, all_block + 12);

			block_index = file_has_used_blocks;
			while(block_index < file_will_use_blocks) {
				ASSERT(all_block[block_index] == 0);

				block_lba = block_bitmap_alloc(current_part);
				if(block_lba == -1) {
					ERROR("block bitmap alloc failed\r");
					rollback_step = 3;
					goto rollback;
				}
				all_block[block_index] = block_lba;
				block_bitmap_index = block_lba - current_part->sb->data_start_lba;
				bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);
				block_index++;
			}
			hd_write(current_part->disk, indirect_block_table, 1, all_block + 12);
		}
	}

	bool first_write_block = true;
	file->fd_pos = file->fd_inode->i_size -1;
	while(bytes_writeen < count) {
		memset(io_buf, 0, BLOCK_SIZE);
		sector_index = file->fd_inode->i_size / BLOCK_SIZE;
		sector_lba = all_block[sector_index];
		sector_offset_bytes = file->fd_inode->i_size % BLOCK_SIZE;
		sector_left_bytes = BLOCK_SIZE - sector_offset_bytes;

		chunk_size = size_left < sector_left_bytes ? size_left : sector_left_bytes;
		if(first_write_block) {
			hd_read(current_part->disk, sector_lba, 1, io_buf);
			first_write_block = false;
		}

		memcpy(io_buf + sector_offset_bytes, src, chunk_size);
		hd_write(current_part->disk, sector_lba, 1, io_buf);

		src += chunk_size;
		bytes_writeen += chunk_size;
		size_left -= chunk_size;
		file->fd_inode->i_size += chunk_size; // bug
		file->fd_pos += chunk_size;
	}

	inode_sync(current_part, file->fd_inode, io_buf);

	kfree(io_buf, BLOCK_SIZE);
	kfree(all_block, 140 * 4);
	return bytes_writeen;

rollback:
	switch(rollback_step) {
		case 3:
			// 分配block过程中出现错误,将已经分配了的block回收
		case 2:
			kfree(all_block, 140 * 4);
		case 1:
			kfree(io_buf, BLOCK_SIZE);
			return -1;
	}
}

int32_t file_read(file_t *file, void *buf, uint32_t count) {
	uint8_t *dst = (uint8_t *)buf;
	uint32_t size = count;
	uint32_t size_left = size;

	if(file->fd_pos + count > file->fd_inode->i_size) {
		size = file->fd_inode->i_size - file->fd_pos;
		size_left = size; // 读剩余
		if(size == 0)
			return -1;
	}

	uint8_t *io_buf = kmalloc(BLOCK_SIZE);
	if(io_buf == NULL) {
		ERROR("kmalloc\r");
		return -1;
	}
	uint32_t *all_block = kmalloc(140 * 4);
	if(all_block == NULL) {
		ERROR("kmalloc\r");
		kfree(io_buf, BLOCK_SIZE);
		return -1;
	}

	uint32_t block_read_start_index = file->fd_pos / BLOCK_SIZE;
	uint32_t block_read_end_index = (file->fd_pos + size) / BLOCK_SIZE;
	uint32_t read_block = block_read_end_index - block_read_start_index;

	int32_t indirect_block_table = 0;
	uint32_t block_index = 0;
	if(read_block == 0) {
		if(block_read_end_index < 12) {
			block_index = block_read_end_index;
			all_block[block_index] = file->fd_inode->i_zone[block_index];
		} else {
			indirect_block_table = file->fd_inode->i_zone[12];
			hd_read(current_part->disk, indirect_block_table, 1, all_block + 12);
		}
	} else {
		if(block_read_end_index < 12) {
			block_index = block_read_start_index;
			while(block_index <= block_read_end_index) {
				all_block[block_index] = file->fd_inode->i_zone[block_index];
				block_index++;
			}
		} else if(block_read_start_index < 12 && block_read_end_index >= 12) {
			block_index = block_read_start_index;
			while(block_index < 12) {
				all_block[block_index] = file->fd_inode->i_zone[block_index];
				block_index++;
			}
			indirect_block_table = file->fd_inode->i_zone[block_index];
			hd_read(current_part->disk, indirect_block_table, 1, all_block + 12);
		} else {
			indirect_block_table = file->fd_inode->i_zone[12];
			hd_read(current_part->disk, indirect_block_table, 1, all_block + 12);
		}
	}

	uint32_t sector_index, sector_lba, sector_offset_bytes, sector_left_bytes, chunk_size;
	uint32_t bytes_read = 0;
	while(bytes_read < size) {
		sector_index = file->fd_pos / BLOCK_SIZE;
		sector_lba = all_block[sector_index];
		sector_offset_bytes = file->fd_pos % BLOCK_SIZE;
		sector_left_bytes = BLOCK_SIZE - sector_offset_bytes;
		chunk_size = size_left < sector_left_bytes ? size_left : sector_left_bytes;

		memset(io_buf, 0, BLOCK_SIZE);
		hd_read(current_part->disk, sector_lba, 1, io_buf);
		memcpy(dst, io_buf + sector_offset_bytes, chunk_size);

		dst += chunk_size;
		file->fd_pos += chunk_size;
		bytes_read += chunk_size;
		size_left -= chunk_size;
	}

	kfree(io_buf, BLOCK_SIZE);
	kfree(all_block, 140 * 4);
	return bytes_read;
}