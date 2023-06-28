#ifndef __FILE_H__
#define __FILE_H__

#include "../stdint.h"

typedef struct inode inode_t;
typedef struct dir dir_t;

#define MAX_FILE_OPEN 64

/* 文件结构 */
typedef struct file {
	uint32_t fd_pos;
	uint32_t fd_flag;
	inode_t * fd_inode;
}file_t;
extern file_t file_table[MAX_FILE_OPEN];

extern int32_t file_create(dir_t *parent_dir, char *filename, uint8_t flag);
extern int32_t file_open(uint32_t inode_no, uint8_t flag);

#endif