#include "../include/kernel/fs.h"
#include "../include/kernel/mm.h"
#include "../include/string.h"
#include "../include/kernel/dir.h"
#include "../include/kernel/file.h"
#include "../include/kernel/inode.h"
#include "../include/kernel/thread.h"

#define TAG "fs"
#include "../include/kernel/debug.h"

#define MAX_FILE_NUMBER 4096

hd_partition_t *current_part;

/* 初始化super block中的位图 */
static void make_bitmap(hd_partition_t *part, uint32_t lba, uint32_t bit_number) {
	uint8_t *buf = kmalloc(SECTOR_SIZE);
	if(buf == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	
	while(bit_number) {
		if(bit_number > BITS_PER_SECTOR) {
			memset(buf, 0, SECTOR_SIZE);
			hd_write_sector(part->disk, lba, 1, buf);
		} else {
			memset(buf, 0, SECTOR_SIZE);
			uint32_t last_byte = bit_number / 8;
			uint32_t last_bit = bit_number % 8;
			uint8_t i = 0;
			memset(buf + last_byte, 0xFF, SECTOR_SIZE - last_byte);
			while(i < last_bit)
				buf[last_byte] &= ~(1 << i++);
			hd_write_sector(part->disk, lba, 1, buf);
			break;
		}

		lba++;
		bit_number -= BITS_PER_SECTOR;
		if(!bit_number)
			break;
	}

	kfree(buf, SECTOR_SIZE);
}

/*
 @brief 分区写入super_block
 @note super_block lba 地址固定 该分区的第二个扇区
 */
static void super_block_init(hd_partition_t *part) {
	uint32_t inode_bitmap_sectors = DIV_ROUND_UP(MAX_FILE_NUMBER, BITS_PER_SECTOR); //inode位图 默认最大存4096个文件
	uint32_t inode_table_sectors = DIV_ROUND_UP(MAX_FILE_NUMBER * sizeof(inode_t), SECTOR_SIZE); //inode数组 4096个文件的inode
	
	uint32_t used_sectors = 2 + inode_bitmap_sectors + inode_table_sectors; //mbr + super_block
	uint32_t free_sectors = part->sector_number - used_sectors;
	
	uint32_t block_bitmap_sectors = DIV_ROUND_UP(free_sectors, BITS_PER_SECTOR);
	uint32_t block_bitmap_bit_len = free_sectors - block_bitmap_sectors;
	block_bitmap_sectors = DIV_ROUND_UP(block_bitmap_bit_len, BITS_PER_SECTOR); //空闲块位图

	super_block_t *sb = kmalloc(sizeof(super_block_t));
	if(sb == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	memset(sb, 0, sizeof(super_block_t));
	sb->magic = FS_EXT;
	sb->lba_base = part->sector_start;
	sb->sector_number = part->sector_number;
	
	sb->block_bitmap_lba = sb->lba_base + 2;
	sb->block_bitmap_sectors = block_bitmap_sectors;

	sb->inode_number = MAX_FILE_NUMBER;
	sb->inode_bitmap_lba = sb->block_bitmap_lba + sb->block_bitmap_sectors;
	sb->inode_bitmap_sectors = inode_bitmap_sectors;

	sb->inode_table_lab = sb->inode_bitmap_lba + sb->inode_bitmap_sectors;
	sb->inode_table_sectors = inode_table_sectors;

	sb->data_start_lba = sb->inode_table_lab + sb->inode_table_sectors;
	sb->root_inode_no = 0xFFAA5500;
	sb->dir_entry_size = sizeof(dir_entry_t);
	
	INFO("%s super block :\r", part->name);
	INFO("    type : %d\r", sb->magic);
	INFO("    lba_base : %d\r", sb->lba_base);
	INFO("    sector_number : %d\r", sb->sector_number);
	INFO("    block_bitmap_lba : %d\r", sb->block_bitmap_lba);
	INFO("    block_bitmap_sectors : %d\r", sb->block_bitmap_sectors);
	INFO("    inode_number : %d\r", sb->inode_number);
	INFO("    inode_bitmap_lba : %d\r", sb->inode_bitmap_lba);
	INFO("    inode_bitmap_sectors : %d\r", sb->inode_bitmap_sectors);
	INFO("    inode_table_sectors : %d\r", sb->inode_table_sectors);
	INFO("    data_start_lba : %d\r", sb->data_start_lba);
	INFO("    dir_entry_size : %d\r", sb->dir_entry_size);

	disk_t *hd = part->disk;
	hd_write_sector(hd, part->sector_start + 1, 1, (void *)sb);
	INFO("super block_bla : %d\r", part->sector_start + 1);

	uint8_t *buf = kmalloc(sizeof(super_block_t));
	if(buf == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	hd_read_sector(hd, part->sector_start + 1, 1, buf);
	PRINT_HEX(buf, 13 * 4);

	make_bitmap(part, sb->block_bitmap_lba, block_bitmap_bit_len);
	make_bitmap(part, sb->inode_bitmap_lba, MAX_FILE_NUMBER);

	kfree(sb, sizeof(super_block_t));
	kfree(buf, sizeof(super_block_t));
}

/*
 @brief 挂载分区
 @note 在内存中创建一份位图,维护
 */
static bool mount_partition(list_item_t *pitem, int arg) {
	char *part_name = (char *)arg;
	hd_partition_t *part = item2entry(hd_partition_t, part_list_item, pitem);

	if(!strcmp(part->name, part_name)) {
		current_part = part;

		disk_t *hd = current_part->disk;
		super_block_t *sb = kmalloc(sizeof(super_block_t));
		if(sb == NULL) {
			PANIC("kmalloc\r");
			return;
		}
		memset(sb, 0, sizeof(super_block_t));
		hd_read_sector(hd, current_part->sector_start + 1, 1, sb);

		/* 读出超级块 */
		current_part->sb = kmalloc(sizeof(super_block_t));
		if(current_part->sb == NULL) {
			PANIC("kmalloc\r");
			return;
		}
		memcpy(current_part->sb, sb, sizeof(super_block_t));

		/* 读出block位图 */
		current_part->block_bitmap.map = kmalloc(sb->block_bitmap_sectors * SECTOR_SIZE);
		if(current_part->block_bitmap.map == NULL) {
			PANIC("kmalloc\r");
			return;
		}
		current_part->block_bitmap.bitmap_byte_len = sb->block_bitmap_sectors * SECTOR_SIZE;
		hd_read_sector(hd, sb->block_bitmap_lba, sb->block_bitmap_sectors, current_part->block_bitmap.map);

		/* 读出inode位图 */
		current_part->inode_bitmap.map = kmalloc(sb->inode_bitmap_sectors * SECTOR_SIZE);
		if(current_part->inode_bitmap.map == NULL) {
			PANIC("kmalloc\r");
			return;
		}
		current_part->inode_bitmap.bitmap_byte_len = sb->inode_bitmap_sectors * SECTOR_SIZE;
		hd_read_sector(hd, sb->inode_bitmap_lba, sb->inode_bitmap_sectors, current_part->inode_bitmap.map);

		list_init(&current_part->open_inodes);
		INFO("mount %s\r", part_name);

		return true;
	}
	
	return false; //继续找
}

/*
 1.写入超级块
 2.初始化位图
 3.挂载分区
 */
void file_system_init(void) {
	uint8_t channel_no = 0, disk_no = 0, part_index = 0;

	super_block_t *sb = kmalloc(sizeof(super_block_t));
	if(sb == NULL) {
		PANIC("kmalloc\r");
		return;
	}

	disk_t *hd = NULL;
	hd_partition_t *part = NULL;
	while(channel_no < channel_number) {
		disk_no = 0;
		while(disk_no < 2) {
			hd = &channels[channel_no].disk[disk_no];
			part = &hd->prim_parts;

			//mark 只有hdb80M.img作了分区,先用第一个分区做文件系统
			if(part->sector_number != 0) {
				memset(sb, 0, 512);

				hd_read_sector(hd, part->sector_start + 1, 1, sb);

				// super_block_init(part); // 重置硬盘

				if(sb->magic == FS_EXT) {
					INFO("%s has filesystem\r", part->name);
				} else {
					super_block_init(part);
				}
			}
			disk_no++;
		}
		channel_no++;
	}

	kfree(sb, sizeof(super_block_t));

	char default_part[] = "ata0-S-1";
	list_traverasl(&partition_list, mount_partition, (int)default_part);
	if(current_part->sb->root_inode_no == 0xFFAA5500)
		create_root_dir(current_part);
}

/*
 @brief 申请一个空闲节点
 @retval 返回节点号 错误返回-1
 */
int32_t inode_bitmap_alloc(hd_partition_t *part) {
	int32_t bit_index = bitmap_continuous_scan(&part->inode_bitmap, 1);
	if(bit_index == -1)
		return -1;
	
	bitmap_set(&part->inode_bitmap, bit_index, bitmap_used);
	return bit_index;
}

/*
 @brief 申请一个空闲块
 @retval 返回扇区地址 错误返回-1
 */
int32_t block_bitmap_alloc(hd_partition_t *part) {
	int32_t bit_index = bitmap_continuous_scan(&part->block_bitmap, 1);
	if(bit_index == -1)
		return -1;
	
	bitmap_set(&part->block_bitmap, bit_index, bitmap_used);
	return part->sb->data_start_lba + bit_index;
}

/*
 @brief 将内存中的bitmap同步到硬盘里
 */
void bitmap_sync(hd_partition_t *part, uint32_t index, bitmap_type_t type) {
	uint32_t offset_sector = index / BITS_PER_SECTOR;
	uint32_t offset_byte = offset_sector * SECTOR_SIZE;

	uint32_t sector_lba; //硬盘位图地址
	uint8_t *offset; //内存位图地址

	switch(type) {
		case BITMAP_INODE:
			sector_lba = part->sb->inode_bitmap_lba + offset_sector;
			offset = part->inode_bitmap.map + offset_byte;
			break;
		case BITMAP_BLOCK:
			sector_lba = part->sb->block_bitmap_lba + offset_sector;
			offset = part->block_bitmap.map + offset_byte;
			break;
	}
	hd_write(part->disk, sector_lba, 1, offset);
}

/*
 @brief 从文件路径中提取最上层文件名并返回剩余路径
 @param pathname 文件路径名
 @param name_store 存储文件名的字符指针
 @retval 剩余路径
 */
char *path_parse(char *pathname, char *name_store) {
	if(pathname[0] == '/') // 跳过根目录
		while(*(++pathname) == '/');
	
	// 提取路径中的文件名
	while(*pathname != '/' && *pathname != '\0')
		*name_store++ = *pathname++;

	if(pathname[0] == '\0')
		return NULL;
	
	return pathname;
}

/*
 @brief 计算文件路径的深度
 @param pathname 文件路径名
 @retval 深度
 */
int32_t path_depth_count(char *pathname) {
	ASSERT(pathname != NULL);
	char *ptr = pathname;
	char name[MAX_FILE_NAME_LEN];
	uint32_t depth = 0;

	ptr = path_parse(ptr, name);
	while(name[0]) {
		depth++;
		memset(name, 0, sizeof(name));
		if(ptr) // 路径不为空,继续提取
			ptr = path_parse(ptr, name);
	}

	return depth;
}

/*
 @brief 查找指定路径的文件或目录
 @param pathname 路径名
 @param searched_record 记录文件搜索的过程和结果
 */
int search_file(const char *pathname, path_search_record_t *searched_record) {
	if(!strcmp(pathname, "/") || !strcmp(pathname, "/.") || !strcmp(pathname, "/..")) { // 跳过根目录
		searched_record->parent_dir = &root_dir;
		searched_record->file_type = FILE_DIRECTORY;
		searched_record->searched_path[0] = '\0';
		return current_part->sb->root_inode_no;
	}

	uint32_t path_len = strlen(pathname);
	ASSERT(pathname[0] == '/' && path_len > 1 && path_len < MAX_PATH_LEN);
	char *sub_path = (char *)pathname;
	dir_t *parent_dir = &root_dir;
	dir_entry_t dir_entry;

	char name[MAX_FILE_NAME_LEN] = {0};

	searched_record->parent_dir = parent_dir;
	searched_record->file_type = FILE_NULL;
	uint32_t parent_inode_no = current_part->sb->root_inode_no; // 备份已经解析的路径的父目录

	sub_path = path_parse(sub_path, name); // 逐级解析路径,name提取的文件名 sub_path:剩余路径
	while(name[0]) {
		ASSERT(strlen(searched_record->searched_path) < MAX_PATH_LEN);

		strcat(searched_record->searched_path, "/");
		strcat(searched_record->searched_path, name);

		if(dir_entry_search(current_part, parent_dir, name, &dir_entry)) {
			if(FILE_DIRECTORY == dir_entry.f_type) {
				parent_inode_no = parent_dir->inode->i_no;
				dir_close(parent_dir);
				parent_dir = dir_open(current_part, dir_entry.inode_no);
				searched_record->parent_dir = parent_dir;
			} else if(FILE_REGULAR == dir_entry.f_type) {
				searched_record->file_type = FILE_REGULAR;
				return dir_entry.inode_no; //找到了文件
			}
		} else {
			return -1; // 找不到
		}

		memset(name, 0, MAX_FILE_NAME_LEN);
		if(sub_path) // 继续解析
			sub_path = path_parse(sub_path, name);
	}

	searched_record->file_type = FILE_DIRECTORY; // 找到了目录
	dir_close(searched_record->parent_dir);
	searched_record->parent_dir = dir_open(current_part, parent_inode_no);

	return dir_entry.inode_no;
}

/*
 @brief 打开或创建文件
 @retval 文件描述符
 */
int32_t sys_open(const char *pathname, uint8_t flag) {
	if(pathname[strlen(pathname) -1] == '/') {
		ERROR("can't open a directory %s\r", pathname);
		return -1;
	}

	int32_t fd = -1;
	uint32_t pathname_depth = path_depth_count(pathname);

	path_search_record_t searched_record = {0};
	int inode_no = search_file(pathname, &searched_record);
	bool found = inode_no == -1 ? false : true;

	if(searched_record.file_type == FILE_DIRECTORY) { // 找到的是目录
		ERROR("can't open a directory\r");
		goto error_return;
	}

	uint32_t searched_path_depth = path_depth_count(searched_record.searched_path);
	if(pathname_depth != searched_path_depth) { // 目录层级有问题，缺少目录
		ERROR("can't access %s : not a directory, subpath %s is't exist\r", pathname, searched_record.searched_path);
		goto error_return;
	}

	if(!found && !(flag & O_CREAT)) { // 没有找到且不是创建文件
		ERROR("in path %s, file %s is't exist\r", searched_record.searched_path, strrchr(searched_record.searched_path, '/') + 1);
		goto error_return;
	} else if(found && (flag & O_CREAT)) { // 找到了但是创建文件
		ERROR("%s has already exist\r", pathname);
		goto error_return;
	}

	switch(flag & O_CREAT)
	{
		case O_CREAT:
			INFO("create file\r");
			fd = file_create(searched_record.parent_dir, strrchr(pathname, '/') + 1, flag);
			dir_close(searched_record.parent_dir);
			break;
		default:
			fd = file_open(inode_no, flag);
	}

	return fd;

error_return:
	dir_close(searched_record.parent_dir);
	return -1;
}

/*
 @brief 删除指定文件
 @param pathname 文件路径
 @retval 0:成功 1:失败
 1.查询该文件是否存在
 2.查询该文件是否被打开
 3.父目录删除该文件的dir_entry
 4.回收该文件block,inode占用的扇区
 */
int32_t sys_unlink(const char *pathname) {
	ASSERT(strlen(pathname) < MAX_PATH_LEN);

	path_search_record_t search_record = {0};
	int inode_no = search_file(pathname, &search_record);
	if(inode_no == -1) {
		ERROR("file %s not found\r", pathname);
		dir_close(search_record.parent_dir);
		return -1;
	}

	if(search_record.file_type == FILE_DIRECTORY) {
		ERROR("can't delete a directory with unlink, use rmdir to insted\r");
		dir_close(search_record.parent_dir);
		return -1;
	}

	uint32_t file_index = 0;
	while(file_index < MAX_FILE_OPEN) {
		if(file_table[file_index].fd_inode != NULL && inode_no == file_table[file_index].fd_inode->i_no)
			break; //查询该文件是否被使用
		file_index++;
	}

	if(file_index < MAX_FILE_OPEN) {
		dir_close(search_record.parent_dir);
		ERROR("file %s is in use, not allow to delete\r", pathname);
		return -1;
	}

	void *buf = kmalloc(SECTOR_SIZE * 2);
	if(buf == NULL) {
		WARN("kmalloc\r");
		return -1;
	}
	
	dir_t *parent_dir = search_record.parent_dir;
	dir_entry_delete(current_part, parent_dir, inode_no, buf);
	inode_release(current_part, inode_no);

	kfree(buf, SECTOR_SIZE * 2);
	dir_close(search_record.parent_dir);
	return 0;
}

/*
 @brief 创建新目录
 @retval 0:成功 -1:失败
 @note 写了四次硬盘
 1.新创建目录分配空闲块写入. ..
 2.父目录block写入新创建目录的dir_entry
 3.父目录inode节点更新
 4.新目录inode节点写入
 
 另外还有位图更新
 */
int32_t sys_mkdir(const char *pathname) {
	uint8_t rollback_step = 0;

	// 1.分配缓冲空间
	void *buf = kmalloc(SECTOR_SIZE * 2);
	if(buf == NULL) {
		WARN("kmalloc\r");
		return -1;
	}
	memset(buf, 0, SECTOR_SIZE * 2);

	// 2.查找目录是否已经存在,确定新目录应该放在哪个父目录下
	path_search_record_t searched_record = {0};
	int inode_no = search_file(pathname, &searched_record);
	if(inode_no != -1) {
		ERROR("file or directory %s exist\r", pathname);
		rollback_step = 1;
		goto rollback;
	} else {
		uint32_t path_depth = path_depth_count(pathname);
		uint32_t searched_path_depth = path_depth_count(searched_record.searched_path);
		if(path_depth != searched_path_depth) {
			ERROR("can't access %s, subpath %s is't exist\r", pathname, searched_record.searched_path);
			rollback_step = 1;
			goto rollback;
		}
	}

	// 3.分配新目录的inode节点和数据块，并将其初始化
	dir_t *parent_dir = searched_record.parent_dir;
	char *dirname = strrchr(pathname, '/') + 1;
	inode_no = inode_bitmap_alloc(current_part);
	if(inode_no == -1) {
		ERROR("allocate inode failed\r");
		rollback_step = 1;
		goto rollback;
	}
	
	inode_t new_dir_inode;
	inode_init(inode_no, &new_dir_inode);

	uint32_t block_bitmap_index = 0;
	int32_t block_lba = -1;
	block_lba = block_bitmap_alloc(current_part);
	if(block_lba == -1) {
		ERROR("allocate block failed\r");
		rollback_step = 2;
		goto rollback;
	}
	new_dir_inode.i_zone[0] = block_lba;
	block_bitmap_index = block_lba - current_part->sb->data_start_lba;
	bitmap_sync(current_part, block_bitmap_index, BITMAP_BLOCK);

	dir_entry_t *p_de = (dir_entry_t *)buf;
	dir_entry_create(".", inode_no, FILE_DIRECTORY, p_de);
	dir_entry_create("..", parent_dir->inode->i_no, FILE_DIRECTORY, p_de + 1);
	hd_write(current_part->disk, new_dir_inode.i_zone[0], 1, (uint8_t *)p_de);
	new_dir_inode.i_size = current_part->sb->dir_entry_size * 2;

	// 4.在父目录中创建一个新的目录项，并将其写入磁盘
	dir_entry_t new_dir_entry = {0};
	dir_entry_create(dirname, inode_no, FILE_DIRECTORY, &new_dir_entry);
	memset(buf, 0, SECTOR_SIZE * 2);
	if(!dir_entry_sync(parent_dir, &new_dir_entry, buf)) {
		ERROR("dir_entry_sync to disk failed\r");
		rollback_step = 2;
		goto rollback;
	}

	// 5.将新目录的inode节点和父目录的inode节点同步到磁盘上
	memset(buf, 0, SECTOR_SIZE * 2);
	inode_sync(current_part, parent_dir->inode, buf);

	memset(buf, 0, SECTOR_SIZE * 2);
	inode_sync(current_part, &new_dir_inode, buf);

	bitmap_sync(current_part, inode_no, BITMAP_INODE);

	kfree(buf, SECTOR_SIZE * 2);
	dir_close(searched_record.parent_dir);
	return 0;

rollback:
	switch(rollback_step) {
		case 2:
			bitmap_set(&current_part->inode_bitmap, inode_no, bitmap_unused);
		case 1:
			dir_close(searched_record.parent_dir);
			break;
	}

	kfree(buf, SECTOR_SIZE * 2);
	return -1;
}

int32_t sys_rmdir(const char *pathname) {
	path_search_record_t search_record = {0};
	uint32_t inode_no = search_file(pathname, &search_record);
	uint32_t ret = -1;
	if(inode_no == -1) {
		ERROR("in %s, sub path %s not exist\r", pathname, search_record.searched_path);
	} else if(search_record.file_type == FILE_REGULAR) {
		ERROR("%s is regular file\r");
	} else {
		dir_t *dir = dir_open(current_part, inode_no);
		if(!dir_is_empty(dir)) {
			ERROR("dir %s is not empty, it is not allowed to delete a nonempty directory\r", pathname);
		} else {
			if(!dir_remove(search_record.parent_dir, dir))
				ret = 0;
		}
		dir_close(dir);
	}

	dir_close(search_record.parent_dir);
	return ret;
}

/*
 @brief 打开一个目录并返回对应的目录结构体指针
 @param 目录路径
 @retval 目录结构体指针 NULL:失败
 */
dir_t *sys_opendir(const char *name) {
	if(strcmp(name, "/") == 0)
		return &root_dir;

	dir_t *dir = NULL;
	path_search_record_t search_record = {0};
	int inode_no = search_file(name, &search_record);
	if(inode_no == -1) {
		ERROR("in path %s, sub path %s is't exist\r", name, search_record.searched_path);
	} else {
		if(search_record.file_type == FILE_REGULAR) {
			ERROR("%s is regular file\r", name);
		} else {
			dir = dir_open(current_part, inode_no);
		}
	}

	dir_close(search_record.parent_dir);
	return dir;
}

int32_t sys_closedir(dir_t *dir) {
	int32_t ret = -1;
	if(dir != NULL) {
		dir_close(dir);
		ret = 0;
	}
	return ret;
}

/*
 @brief 获取当前进程的工作路径
 @param buf 存储工作路径
 @param size buf的长度
 */
char *sys_getcwd(char *buf, uint32_t size) {
	task_t *current = running_thread();
	int32_t parent_inode_no = 0;
	int32_t child_inode_no = current->cwd_inode_no;
	if(child_inode_no == current_part->sb->root_inode_no) { // 根目录
		strcpy(buf, "/");
		return buf;
	}

	void *io_buf = kmalloc(SECTOR_SIZE + MAX_PATH_LEN);
	if(io_buf == NULL) {
		WARN("kmalloc\r");
		return NULL;
	}
	memset(io_buf, 0, SECTOR_SIZE + MAX_PATH_LEN);
	char *full_path = io_buf + SECTOR_SIZE;

	memset(buf, 0, size);
	while(child_inode_no != current_part->sb->root_inode_no) { //往上找父目录,直到找到根目录
		parent_inode_no = get_parent_dir_inode_no(child_inode_no, io_buf);
		if(get_child_dir_name(parent_inode_no, child_inode_no, full_path, io_buf) == -1) {
			kfree(io_buf, SECTOR_SIZE + MAX_PATH_LEN);
			return NULL;
		}
		child_inode_no = parent_inode_no;
	}

	ASSERT(strlen(full_path) <= size);
	char *last_slash;
	while(last_slash = strrchr(full_path, '/')) { // 将full_path路径名反转
		uint32_t len = strlen(buf);
		strcpy(buf + len, last_slash);
		*last_slash = '\0'; // 将’/‘更改为’\0‘,继续查找
	}

	kfree(io_buf, SECTOR_SIZE + MAX_PATH_LEN);
	return buf;
}

/*
 @brief 改变当e工作目录
 @param path 目录的绝对地址
 @retval 0:成功 -1:失败
 */
int32_t sys_chdir(const char *path) {
	int32_t ret = -1;
	path_search_record_t search_record = {0};
	int32_t inode_no = search_file(path, &search_record);
	if(inode_no != -1) {
		if(search_record.file_type == FILE_DIRECTORY) {
			running_thread()->cwd_inode_no = inode_no;
			ret = 0;
		} else {
			ERROR("%s is regular file\r");
		}
	}
	dir_close(search_record.parent_dir);
	return ret;
}