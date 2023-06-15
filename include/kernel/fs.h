/*
 初始化文件系统:写入super block
 1.固定地址找到super block
 2.根据super block:确定块位图 inode数组位图 根目录
 3.块位图:空闲块分配
 4.inode位图:文件/目录管理 i_zone指向文件所属块 目录存放目录项
 5.根目录 文件系统初始化时创建
 */

#ifndef __FS_H__
#define __FS_H__

#include "../stdint.h"
#include "./list.h"

#define MAX_FILE_NAME_LEN 16
#define PAGE_SIZE 4096
#define SECTOR_SIZE 512
#define BITS_PER_SECTOR (SECTOR_SIZE * 8)

typedef enum file_type {
	FILE_REGULAR = 1, //文件
	FILE_DIRECTORY, //目录
}file_type_t;

typedef enum fs_type {
	FS_EXT = 1,
	FS_FAT32,
	FS_NTFS,
}fs_type_t;

typedef struct super_block {
	uint32_t magic; //文件系统类型
	
	uint32_t lba_base; //分区起始lba地址
	uint32_t sector_number; //分区扇区数

	uint32_t block_bitmap_lba; //块位图起始扇区地址
	uint32_t block_bitmap_sectors; //位图占用扇区数

	uint32_t inode_number; //分区inode数
	uint32_t inode_bitmap_lba; //inode位图起始扇区地址
	uint32_t inode_bitmap_sectors; //位图占用扇区数

	uint32_t inode_table_lab; //inode数组起始扇区数
	uint32_t inode_table_sectors; //数组占用扇区数

	uint32_t data_start_lba; //可用数据区起始lba地址
	uint32_t root_inode_no; //根目录所在inode
	uint32_t dir_entry_size; //目录项大小

	uint8_t padding[512 - 13 * 4];
}__attribute__((packed)) super_block_t;

typedef struct inode {
	uint32_t i_no; //inode 编号
	uint32_t i_mode; //属性 rwx
	uint32_t i_size; //大小
	uint32_t i_time; //修改时间
	uint32_t i_open_counts;
	uint32_t i_zone[13]; //文件
	list_item_t i_list_item;
	bool i_write_deny;
}__attribute__((packed)) inode_t;

typedef struct dir_entry {
	char filename[MAX_FILE_NAME_LEN];
	uint32_t inode_no; //inode 编号
	file_type_t f_type; //文件类型
}__attribute__((packed)) dir_entry_t;

extern void file_system_init(void);

#endif