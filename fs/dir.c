#include "../include/kernel/dir.h"
#include "../include/kernel/mm.h"
#include "../include/kernel/inode.h"
#include "../include/string.h"

#include "../include/kernel/debug.h"

dir_t root_dir;

/* 创建目录项 */
void dir_entry_create(char *fliename, uint32_t inode_no, file_type_t file_type, dir_entry_t *p_de) {
	ASSERT(strlen(fliename) < MAX_FILE_NAME_LEN)

	memcpy(p_de->filename, fliename, strlen(fliename));
	p_de->f_type = file_type;
	p_de->inode_no = inode_no;
}

/* 创建根目录 */
void create_root_dir(hd_partition_t *part) {
	inode_t *inode = kmalloc(SECTOR_SIZE);
	if(inode == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	memset(inode, 0, SECTOR_SIZE);
	
	int32_t inode_index = inode_bitmap_alloc(part);
	inode_init(inode_index, inode);
	part->sb->root_inode_no = inode->i_no;

	inode->i_zone[0] = block_bitmap_alloc(part);
	dir_entry_t *new_dir_entry = kmalloc(SECTOR_SIZE);
	if(new_dir_entry == NULL) {
		PANIC("kmalloc\r");
		return;
	}
	memset(new_dir_entry, 0, SECTOR_SIZE);

	dir_entry_create(".", inode_index, FILE_DIRECTORY, new_dir_entry);
	dir_entry_create("..", inode_index, FILE_DIRECTORY, new_dir_entry + 1);
	inode->i_size = part->sb->dir_entry_size * 2;

	inode_sync(part, inode, inode); //更新inode

	hd_write(part->disk, inode->i_zone[0], 1, new_dir_entry); //写入文件

	bitmap_sync(part, inode->i_zone[0] - part->sb->data_start_lba, BITMAP_BLOCK); //更新block位图

	bitmap_sync(part, inode_index, BITMAP_INODE); //更新inode位图

	kfree(inode, SECTOR_SIZE);
	kfree(new_dir_entry, SECTOR_SIZE);
}

void open_root_dir(hd_partition_t *part) {
	root_dir.inode = inode_open(part, part->sb->root_inode_no);
	root_dir.dir_pos = 0;
}

