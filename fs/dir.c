#include "../include/kernel/dir.h"
#include "../include/kernel/mm.h"
#include "../include/kernel/inode.h"
#include "../include/string.h"

#include "../include/kernel/debug.h"

dir_t root_dir;

/*
 @brief 创建目录项
 @param filename 文件名
 @param inode_no 该文件的inode索引
 @param file_type 该文件类型
 @param p_de dir_entry地址
 */
void dir_entry_create(char *filename, uint32_t inode_no, file_type_t file_type, dir_entry_t *p_de) {
	ASSERT(strlen(filename) < MAX_FILE_NAME_LEN)

	memcpy(p_de->filename, filename, strlen(filename));
	p_de->f_type = file_type;
	p_de->inode_no = inode_no;
}

/*
 @brief 将目录项写入到父目录中
 @param parent_dir 父目录
 @param p_de 目录项
 @param buf 缓冲区
 @retal false:写入失败 true:成功
 */
bool dir_entry_sync(dir_t *parent_dir, dir_entry_t *p_de, void *buf) {
	inode_t *parent_dir_inode = parent_dir->inode;
	uint32_t dir_size = parent_dir_inode->i_size;
	uint32_t dir_entry_size = current_part->sb->dir_entry_size;
	uint32_t dir_entrys_per_sector = SECTOR_SIZE / dir_entry_size;
	
	int32_t block_lba = -1;
	uint8_t block_index = 0;
	uint32_t all_block[140] = {0};

	while(block_index < 12) { // 拿到已有块
		all_block[block_index] = parent_dir_inode->i_zone[block_index];
		block_index++;
	}
	if(parent_dir_inode->i_zone[block_index] != 0)
		hd_read(current_part->disk, parent_dir_inode->i_zone[block_index], 1, &all_block[block_index]);

	dir_entry_t *dir_entry = (dir_entry_t *)buf;
	int32_t block_bitmap_index = -1;

	block_index = 0;
	while(block_index < 140) {
		block_bitmap_index = -1;
		if(all_block[block_index] == 0) { // 没有分配过块,新分配并写入
			block_lba = block_bitmap_alloc(current_part);
			if(block_lba == -1) {
				WARN("alloc block bitmap failed\r");
				return false;
			}

			block_bitmap_index = block_lba - current_part->sb->data_start_lba;
			bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);

			block_bitmap_index = -1;
			if(block_index < 12) { // 直接块
				parent_dir_inode->i_zone[block_index] = all_block[block_index] = block_lba;
			} else if(block_index == 12) { // 一级间接块
				parent_dir_inode->i_zone[block_index] = block_lba; // 之前申请的block当作一级间接块
				
				block_lba = -1;
				block_lba = block_bitmap_alloc(current_part); // 再申请一块用于存储
				if(block_lba == -1) {
					block_bitmap_index = parent_dir_inode->i_zone[block_index] - current_part->sb->data_start_lba;
					bitmap_set(&current_part->block_bitmap, block_bitmap_index, bitmap_unused);
					parent_dir_inode->i_zone[block_index] = 0;

					WARN("alloc block bitmap failed\r");
					return false;
				}

				block_bitmap_index = block_lba - current_part->sb->data_start_lba;
				bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);

				all_block[block_index] = block_lba;
				hd_write(current_part->disk, all_block[block_index], 1, &all_block[block_index]);
			} else { //一级间接块存储的直接块
				all_block[block_index] = block_lba;
				hd_write(current_part->disk, all_block[block_index], 1, &all_block[block_index]);
			}
			
			memset(buf, 0, SECTOR_SIZE);
			memcpy(buf, p_de, dir_entry_size);
			hd_write(current_part->disk, all_block[block_index], 1, (uint8_t *)buf);  //写入

			parent_dir_inode->i_size += dir_entry_size;
			return true;
		}

		// 已分配过,在块中找空闲写入
		hd_read(current_part->disk, all_block[block_index], 1, (uint8_t *)dir_entry);
		uint8_t dir_entry_index = 0;
		while(dir_entry_index < dir_entrys_per_sector) {
			if((dir_entry + dir_entry_index)->f_type == FILE_NULL) {
				memcpy(dir_entry + dir_entry_index, p_de, dir_entry_size);
				hd_write(current_part->disk, all_block[block_index], 1, (uint8_t *)dir_entry);  //写入

				parent_dir_inode->i_size += dir_entry_size;
				return true;
			}
			dir_entry_index++;
		}

		block_index++;
	}

	WARN("dir entry is full\r");
	return false;
}

/*
 @brief 在指定目录中查找指定文件名的目录项
 @param parent_dir 父目录
 @param name 要查找的文件名
 @param dir_entry 缓冲区
 @retval false:没找到 true:找到了
 */
bool dir_entry_search(hd_partition_t *part, dir_t *parent_dir, const char *name, dir_entry_t *dir_entry) {
	uint32_t block_number = 12;
	uint32_t all_block[140] = {0};
	uint32_t block_index = 0;
	inode_t *parent_dir_inode = parent_dir->inode;

	while(block_index < block_number) {
		all_block[block_index] = parent_dir_inode->i_zone[block_index];
		block_index++;
	}
	if(parent_dir_inode->i_zone[block_index] != 0) {
		hd_read(part->disk, parent_dir_inode->i_zone[block_index], 1, &all_block[block_index]);
		block_number = 140;
	}
	block_index = 0;

	uint8_t *buf = kmalloc(SECTOR_SIZE);
	if(buf == NULL) {
		WARN("kmalloc");
		return false;
	}
	dir_entry_t *p_de = (dir_entry_t *)buf;
	uint32_t dir_entry_size = part->sb->dir_entry_size;
	uint32_t dir_entrys_per_sector = SECTOR_SIZE / dir_entry_size;
	uint32_t dir_entry_index = 0;

	while(block_index < block_number) {
		if(all_block[block_index] == 0) {
			block_index++;
			continue;
		}

		hd_read(part->disk, all_block[block_index], 1, (uint8_t *)p_de);

		dir_entry_index = 0;
		while(dir_entry_index < dir_entrys_per_sector) {
			if(!strcmp(p_de->filename, name)) {
				memcpy(dir_entry, p_de, dir_entry_size); // 找到了

				kfree(buf, SECTOR_SIZE);
				return true;
			}

			dir_entry_index++;
			p_de++;
		}

		block_index++;
		p_de = (dir_entry_t *)buf;
		memset(p_de, 0, SECTOR_SIZE);
	}

	kfree(buf, SECTOR_SIZE);
	return false;
}

/*
 @brief 删除目录下指定文件的目录项
 @param parent_dir 目录
 @param inode_no 要删除文件的inode_no
 @param buf 缓冲区
 @retval true:成功 false:失败
 */
bool dir_entry_delete(hd_partition_t *part, dir_t* parent_dir, uint32_t inode_no, void *buf) {
	inode_t *parent_dir_inode = parent_dir->inode;
	uint32_t block_number = 12;
	uint32_t block_index = 0;
	uint32_t all_block[140] = {0};
	
	while(block_index < block_number) {
		all_block[block_index] = parent_dir_inode->i_zone[block_index];
		block_index++;
	}
	if(parent_dir_inode->i_zone[block_index] != 0) {
		hd_read(part->disk, parent_dir_inode->i_zone[block_index], 1, &all_block[block_index]);
		block_number = 140;
	}
	block_index = 0;

	dir_entry_t *p_de = (dir_entry_t *)buf;
	uint32_t dir_entry_size = part->sb->dir_entry_size;
	uint32_t dir_entrys_per_sector = SECTOR_SIZE / dir_entry_size;
	uint32_t dir_entry_index, dir_entry_count;
	bool is_dir_first_block = false;
	dir_entry_t *dir_entry_found = NULL;

	while(block_index < block_number) {
		is_dir_first_block = false;
		if(all_block[block_index] == 0) {
			block_index++;
			continue;
		}
		
		dir_entry_index = dir_entry_count = 0;
		memset(buf, 0, SECTOR_SIZE);
		hd_read(part->disk, all_block[block_index], 1, buf);
		while(dir_entry_index < dir_entrys_per_sector) {
			if((p_de + dir_entry_index)->f_type != FILE_NULL) {
				if(!strcmp((p_de + dir_entry_index)->filename, "."))
					is_dir_first_block = true;
				else if(strcmp((p_de + dir_entry_index)->filename, ".") && strcmp((p_de + dir_entry_index)->filename, ".")) {
					dir_entry_count++; // 统计目录项个数,用于释放block
					if((p_de + dir_entry_index)->inode_no == inode_no)
						dir_entry_found = p_de + dir_entry_index; // 找到了
				}
			}
			dir_entry_index++;
		}

		if(dir_entry_found == NULL) {
			block_index++;
			continue; // 继续找
		}

		if(dir_entry_count == 1 && !is_dir_first_block) { // 回收block
			uint32_t block_bitmap_index = all_block[block_index] - part->sb->data_start_lba;
			bitmap_set(&part->block_bitmap, block_bitmap_index, bitmap_unused);
			bitmap_sync(part, block_bitmap_index, BITMAP_BLOCK);

			if(block_index < 12)
				parent_dir_inode->i_zone[block_index] = 0;
			else { //一级间接块
				uint32_t indirect_block = 0;
				uint32_t indirect_block_index = 12;
				while(indirect_block_index < block_number) {
					if(all_block[indirect_block_index] != 0)
						indirect_block++;
				}

				if(indirect_block > 1) {
					all_block[block_index] = 0;
					hd_write(part->disk, parent_dir_inode->i_zone[block_index], 1, (uint8_t *)(all_block + 12));
				} else {
					block_bitmap_index = parent_dir_inode->i_zone[12] - part->sb->data_start_lba;
					bitmap_set(&part->block_bitmap, block_bitmap_index, bitmap_unused);
					bitmap_sync(part, block_bitmap_index, BITMAP_BLOCK);
					parent_dir_inode->i_zone[12] = 0;
				}
			}
		} else { // 只清除这一项dir_entry
			memset(dir_entry_found, 0, dir_entry_size);
			hd_write(part->disk, all_block[block_index], 1, buf);
		}

		parent_dir_inode->i_size -= dir_entry_size;
		memset(buf, 0, SECTOR_SIZE * 2);
		inode_sync(part, parent_dir_inode, buf);

		return true;
	}

	return false;
}

/*
 @brief 获取child_inode_no的父目录的inode编号
 @param child_inode_no 子目录inode编号
 @param buf 缓冲区
 @retval 父目录的inode编号
 */
uint32_t get_parent_dir_inode_no(uint32_t child_inode_no, void *buf) {
	inode_t *child_dir_inode = inode_open(current_part, child_inode_no);
	uint32_t block_lba = child_dir_inode->i_zone[0]; // 找..
	inode_close(child_dir_inode);
	hd_read(current_part->disk, block_lba, 1, buf);
	dir_entry_t *dir_entry = (dir_entry_t *)buf;
	return dir_entry[1].inode_no;
}

/*
 @brief 获取指定父目录下的指定子目录的目录名
 @param path 存储子目录的目录名
 @param buf 缓冲区
 @retval 0:成功获取 -1:未成功
 */
uint32_t get_child_dir_name(uint32_t parent_inode_no, uint32_t child_inode_no, char *path, void *buf) {
	inode_t *parent_dir_inode = inode_open(current_part, parent_inode_no);
	uint8_t block_index = 0;
	uint32_t all_block[140] = {0};
	uint32_t block_number = 12;

	while(block_index < block_number) {
		all_block[block_index] = parent_dir_inode->i_zone[block_index];
		block_index++;
	}
	if(parent_dir_inode->i_zone[block_index]) {
		hd_read(current_part->disk, parent_dir_inode->i_zone[block_index], 1, &all_block[block_index]);
		block_number = 140;
	}
	block_index = 0;

	dir_entry_t *dir_entry = (dir_entry_t *)buf;
	uint32_t dir_entry_size = current_part->sb->dir_entry_size;
	uint32_t dir_entrys_per_sector = SECTOR_SIZE / dir_entry_size;
	uint32_t dir_entry_index = 0;
	while(block_index < block_number) {
		if(all_block[block_index]) {
			memset(buf, 0, SECTOR_SIZE);
			hd_read(current_part->disk, all_block[block_index], 1, buf);

			dir_entry_index = 0;
			while(dir_entry_index < dir_entrys_per_sector) {
				if((dir_entry + dir_entry_index)->inode_no == child_inode_no) {
					strcat(path, "/");
					strcat(path, (dir_entry + dir_entry_index)->filename);
					return 0;
				}
				dir_entry_index++;
			}
		}
		block_index++;
	}

	return -1;
}

/* 创建根目录 */
void create_root_dir(hd_partition_t *part) {
	uint8_t *buf = kmalloc(SECTOR_SIZE);
	if(buf == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	memset(buf, 0, SECTOR_SIZE);

	inode_t new_inode = {0};
	
	int32_t inode_index = inode_bitmap_alloc(part);
	inode_init(inode_index, &new_inode);
	part->sb->root_inode_no = new_inode.i_no;

	new_inode.i_zone[0] = block_bitmap_alloc(part);
	dir_entry_t *new_dir_entry = kmalloc(SECTOR_SIZE);
	if(new_dir_entry == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	memset(new_dir_entry, 0, SECTOR_SIZE);

	dir_entry_create(".", inode_index, FILE_DIRECTORY, new_dir_entry);
	dir_entry_create("..", inode_index, FILE_DIRECTORY, new_dir_entry + 1);
	new_inode.i_size = part->sb->dir_entry_size * 2;

	inode_sync(part, &new_inode, buf); //更新inode

	hd_write(part->disk, new_inode.i_zone[0], 1, new_dir_entry); //写入文件

	bitmap_sync(part, new_inode.i_zone[0] - part->sb->data_start_lba, BITMAP_BLOCK); //更新block位图

	bitmap_sync(part, inode_index, BITMAP_INODE); //更新inode位图

	memset(buf, 0, SECTOR_SIZE);
	memcpy(buf, current_part->sb, sizeof(super_block_t));
	hd_write(part->disk, part->sector_start + 1, 1, buf); // superblock root_inode_no

	kfree(buf, SECTOR_SIZE);
	kfree(new_dir_entry, SECTOR_SIZE);
}

void open_root_dir(hd_partition_t *part) {
	root_dir.inode = inode_open(part, part->sb->root_inode_no);
	root_dir.dir_pos = 0;
}

/*
 @brief 将目录指针重置为目录开头
 @param dir 需要重置的目录
 */
void dir_rewinddir(dir_t *dir) {
	dir->dir_pos = 0;
}

bool dir_is_empty(dir_t *dir) {
	inode_t *dir_inode = dir->inode;
	return (dir_inode->i_size == current_part->sb->dir_entry_size * 2);
}

/*
 @brief 读取目录项
 @param dir 需要读取的目录
 @retval 目录项地址
 @note 需要确保第一次的dir_pos = 0
 */
dir_entry_t *dir_read(dir_t *dir) {
	dir_entry_t *dir_entry = (dir_entry_t *)dir->dir_buf;
	inode_t *dir_inode = dir->inode;
	
	uint32_t all_block[140] = {0};
	uint32_t block_number = 12;
	uint32_t block_index = 0;
	uint32_t dir_entry_index = 0;

	while(block_index < block_number) {
		all_block[block_index] = dir_inode->i_zone[block_index];
		block_index++;
	}
	if(dir_inode->i_zone[block_index] != 0) {
		hd_read(current_part->disk, dir_inode->i_zone[block_index], 1, &all_block[block_index]);
		block_number = 140;
	}
	block_index = 0;

	uint32_t curret_dir_entry_pos = 0;
	uint32_t dir_entry_size = current_part->sb->dir_entry_size;
	uint32_t dir_entrys_per_sector = SECTOR_SIZE / dir_entry_size;
	while(block_index < block_number) { // 遍历所有目录项
		if(dir->dir_pos >= dir_inode->i_size) // 超过了目录的大小
			return NULL;
		
		if(all_block[block_index] == 0) { // 没有块
			block_index++;
			continue;
		}

		memset(dir_entry, 0, SECTOR_SIZE);
		hd_read(current_part->disk, all_block[block_index], 1, (uint8_t *)dir_entry);
		dir_entry_index = 0;
		while(dir_entry_index < dir_entrys_per_sector) { // 查找下一个未使用的目录项
			if((dir_entry + dir_entry_index)->f_type) {
				if(curret_dir_entry_pos < dir->dir_pos) {
					curret_dir_entry_pos += dir_entry_size;
					dir_entry_index++;
					continue;
				}

				dir->dir_pos += dir_entry_size;
				return dir_entry + dir_entry_index;
			}
			dir_entry_index++;
		}
		block_index++;
	}

	return NULL;
}

/*
 @brief 删除指定目录下的子目录
 @param parent_dir 指定目录
 @param child_dir 要删除的子目录
 @retval 0:成功 -1:失败
 */
uint32_t dir_remove(dir_t *parent_dir, dir_t * child_dir) {
	inode_t *child_dir_inode = child_dir->inode;
	int32_t block_index = 1;
	while(block_index < 13) {
		if(child_dir_inode->i_zone[block_index] != 0) {
			ERROR("dir is not empty\r");
			return -1;
		}
		block_index++;
	}

	void *buf = kmalloc(SECTOR_SIZE * 2);
	if(buf == NULL) {
		WARN("kmalloc");
		return -1;
	}

	dir_entry_delete(current_part, parent_dir, child_dir_inode->i_no, buf);

	inode_release(current_part, child_dir_inode->i_no);

	kfree(buf, SECTOR_SIZE * 2);
	return 0;
}

dir_t *dir_open(hd_partition_t *part, uint32_t inode_no) {
	dir_t *dir = kmalloc(sizeof(dir_t));
	if(dir == NULL) {
		WARN("kmalloc");
		return NULL;
	}
	dir->inode = inode_open(part, inode_no);
	dir->dir_pos = 0;
	
	return dir;
}

void dir_close(dir_t *dir) {
	if(dir == &root_dir)
		return;
	
	inode_close(dir->inode);
	kfree(dir, sizeof(dir_t));
}
