#ifndef __DIR_H__
#define __DIR_H__

#include "../stdint.h"

typedef struct dir_entry dir_entry_t;
typedef struct inode inode_t;
typedef struct hd_partition hd_partition_t;

typedef struct dir {
	inode_t *inode;
	uint32_t dir_pos;
	uint8_t dir_buf[512];
}dir_t;
extern dir_t root_dir;

extern void dir_entry_create(char *fliename, uint32_t inode_no, uint32_t file_type, dir_entry_t *p_de);
extern bool dir_entry_sync(dir_t *parent_dir, dir_entry_t *p_de, void *buf);
extern bool dir_entry_search(hd_partition_t *part, dir_t *parent_dir, const char *name, dir_entry_t *dir_entry);
extern bool dir_entry_delete(hd_partition_t *part, dir_t* parent_dir, uint32_t inode_no, void *buf);

extern uint32_t get_parent_dir_inode_no(uint32_t child_inode_no, void *buf);
extern uint32_t get_child_dir_name(uint32_t parent_inode_no, uint32_t child_inode_no, char *path, void *buf);

extern void create_root_dir(hd_partition_t *part);
extern void open_root_dir(hd_partition_t *part);

extern void dir_rewinddir(dir_t *dir);
extern bool dir_is_empty(dir_t *dir);
extern dir_entry_t *dir_read(dir_t *dir);
extern uint32_t dir_remove(dir_t *parent_dir, dir_t *child_dir);
extern dir_t *dir_open(hd_partition_t *part, uint32_t inode_no);
extern void dir_close(dir_t *dir);

#endif