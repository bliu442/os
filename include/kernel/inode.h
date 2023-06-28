#ifndef __INODE_H__
#define __INODE_H__

#include "../stdint.h"
#include "./hd.h"
#include "./fs.h"

extern void inode_init(uint32_t inode_no, inode_t *new_inode);
extern void inode_sync(hd_partition_t *part, inode_t *inode, void *buf);
extern void inode_release(hd_partition_t *part, uint32_t inode_no);
extern inode_t *inode_open(hd_partition_t *part, uint32_t inode_no);
extern void inode_close(inode_t *inode);

#endif