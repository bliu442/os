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
#define MAX_PATH_LEN 512

#define PAGE_SIZE 4096
#define SECTOR_SIZE 512
#define BITS_PER_SECTOR (SECTOR_SIZE * 8)
#define BLOCK_SIZE SECTOR_SIZE

typedef struct hd_partition hd_partition_t;
extern hd_partition_t *current_part;

typedef struct dir dir_t;

enum {
	O_RDONLY = 0,
	O_WRONLY = 1,
	O_RDWR = 2,
	O_CREAT = 4,
};

typedef enum file_type {
	FILE_NULL = 0,
	FILE_REGULAR, //文件
	FILE_DIRECTORY, //目录
}file_type_t;

typedef enum fs_type {
	FS_EXT = 1,
	FS_FAT32,
	FS_NTFS,
}fs_type_t;

typedef enum bitmap_type {
	BITMAP_INODE = 1,
	BITMAP_BLOCK,
}bitmap_type_t;

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
	uint32_t i_size; //文件大小
	uint32_t i_time; //修改时间
	uint32_t i_zone[13]; //文件 0-11存直接块(存储的就是文件数据),12存一级间接块(存储直接块地址) 二级间接块(存储一级间接块地址) [13] = 12 + (512 / 4);[14] = 12 + (512 / 4) + (512 / 4) * (512 / 4) [15]...
	list_item_t i_list_item;
	uint32_t i_open_counts; //统计文件打开次数
	bool i_write_deny;
}__attribute__((packed)) inode_t;

typedef struct dir_entry {
	char filename[MAX_FILE_NAME_LEN];
	uint32_t inode_no; //inode 编号
	file_type_t f_type; //文件类型
}__attribute__((packed)) dir_entry_t;

/* 记录查找文件过程 */
typedef struct path_search_record {
	char searched_path[MAX_PATH_LEN]; // 记录已经找的路径 
	dir_t *parent_dir; // 已经找的路径的父目录(已经打开)
	file_type_t file_type;
}path_search_record_t;

extern void file_system_init(void);

extern int32_t inode_bitmap_alloc(hd_partition_t *part);
extern int32_t block_bitmap_alloc(hd_partition_t *part);
extern void bitmap_sync(hd_partition_t *part, uint32_t index, bitmap_type_t type);

extern char *path_parse(char *pathname, char *name_store);
extern int32_t path_depth_count(char *pathname);

extern int32_t sys_open(const char *pathname, uint8_t flag);
extern int32_t sys_close(int32_t fd);
extern int32_t sys_unlink(const char *pathname);
extern ssize_t sys_write(int fd, const void *buf, size_t count);

extern int32_t sys_mkdir(const char *pathname);
extern int32_t sys_rmdir(const char *pathname);
extern dir_t *sys_opendir(const char *name);
extern int32_t sys_closedir(dir_t *dir);
extern char *sys_getcwd(char *buf, uint32_t size);
extern int32_t sys_chdir(const char *path);

#endif