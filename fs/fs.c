#include "../include/kernel/fs.h"
#include "../include/kernel/mm.h"
#include "../include/string.h"
#include "../include/kernel/dir.h"

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
	sb->root_inode_no = 0;
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
	INFO("    root_inode_no : %d\r", sb->root_inode_no);
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

				super_block_init(part);
				// if(sb->magic == FS_EXT) {
				// 	INFO("%s has filesystem\r", part->name);
				// } else {
				// 	super_block_init(part);
				// }
			}
			disk_no++;
		}
		channel_no++;
	}

	kfree(sb, sizeof(super_block_t));

	char default_part[] = "ata0-S-1";
	list_traverasl(&partition_list, mount_partition, (int)default_part);
	create_root_dir(current_part);
	open_root_dir(current_part);
}

/*
 @brief 申请一个空闲节点
 @retval 返回节点号
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
 @retval 返回扇区地址
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