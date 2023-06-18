#ifndef __DIR_H__
#define __DIR_H__

#include "../stdint.h"
#include "./fs.h"
#include "./hd.h"

typedef struct dir {
	inode_t *inode;
	uint32_t dir_pos;
	uint8_t dir_buf[512];
}dir_t;

extern void create_root_dir(hd_partition_t *part);
extern void open_root_dir(hd_partition_t *part);

#endif