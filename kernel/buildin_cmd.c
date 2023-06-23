#include "../include/kernel/buildin_cmd.h"
#include "../include/kernel/dir.h"
#include "../include/kernel/inode.h"

#include "../include/kernel/debug.h"


void buildin_ls(void) {
	dir_entry_t *dir_entry = NULL;

	dir_rewinddir(&root_dir);
	while(dir_entry = dir_read(&root_dir)) {
		printk("inode_no:%d  filetype:%s  name:%s\r", dir_entry->inode_no, dir_entry->f_type == 1 ? "regular" : "directory", dir_entry->filename);
	}
}