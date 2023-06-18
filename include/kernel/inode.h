#ifndef __INODE_H__
#define __INODE_H__

#include "../stdint.h"
#include "./hd.h"
#include "./fs.h"

void inode_sync(hd_partition_t *part, inode_t *inode, void *buf);
inode_t *inode_open(hd_partition_t *part, uint32_t inode_no);
void inode_close(inode_t *inode);
void inode_init(uint32_t inode_no, inode_t *new_inode);

#endif