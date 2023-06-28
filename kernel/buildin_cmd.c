#include "../include/kernel/buildin_cmd.h"
#include "../include/kernel/dir.h"
#include "../include/kernel/inode.h"
#include "../include/string.h"
#include "../include/kernel/mm.h"

#include "../include/kernel/debug.h"

char final_path[MAX_PATH_LEN];

/*
 @brief 标准化路径
 */
void wash_path(char *old_path, char *new_path) {
	char name[MAX_FILE_NAME_LEN] = {0};
	char *sub_path = old_path;
	sub_path = path_parse(sub_path, name);
	if(name[0] == '\0') {
		strcpy(new_path, "/");
		return;
	}

	new_path[0] = '\0';
	strcat(new_path, "/"); // 从根目录开始
	while(name[0]) {
		if(!strcmp("..", name)) { // 上一层目录,将最后的目录删除
			char *slash = strrchr(new_path, '/');
			if(slash != new_path)
				*slash = '\0';
			else
				*(slash + 1) = '\0';
		} else if(strcmp(".", name)) { // 不是当前目录,将/name拼上去
			if(strcmp(new_path, "/"))
				strcat(new_path, "/");
			strcat(new_path, name);
		}

		memset(name, 0, sizeof(name));
		if(sub_path)
			sub_path = path_parse(sub_path, name);
	}
}

/*
 @brief 获取标准绝对路径
 @param path 路径字符串
 @param final_path 存储标准化的路径字符串
 */
int32_t make_clear_abs_path(char *path, char *final_path) {
	char *abs_path = kmalloc(MAX_PATH_LEN);
	if(abs_path == NULL) {
		ERROR("kmalloc\r");
		return -1;
	}

	if(path[0] != '/') { // 相对路径转换为绝对路径
		memset(abs_path, 0, MAX_PATH_LEN);
		if(sys_getcwd(abs_path, MAX_PATH_LEN) != NULL) {
			if(strcmp(abs_path, "/") != 0)
				strcat(abs_path, "/");
		}
	}

	strcat(abs_path, path);
	wash_path(abs_path, final_path);

	kfree(abs_path, MAX_PATH_LEN);
	return 0;
}

void buildin_ls(uint32_t argc, char **argv) {
	char *pathname = NULL;
	uint32_t arg_path_number = 0;
	uint32_t arg_index = 1;

	bool long_info = false;
	while(arg_index < argc) {
		if(argv[arg_index][0] == '-') { // 参数
			if(strcmp(argv[arg_index], "-l") == 0)
				long_info = true;
		} else { // 指定路径
			if(arg_path_number == 0) {
				pathname = argv[arg_index];
				arg_path_number = 1;
			} else {
				ERROR("only support one path\r");
				return;
			}
		}
		arg_index++;
	}

	if(pathname == NULL) {
		if(NULL != sys_getcwd(final_path, MAX_PATH_LEN))
			pathname = final_path;
		else
			ERROR("getcwd for default path failed\r");
	} else {
		make_clear_abs_path(pathname, final_path);
		pathname = final_path;
	}

	dir_t *dir = sys_opendir(pathname);
	dir_entry_t *dir_entry = NULL;
	dir_rewinddir(dir);
	while(dir_entry = dir_read(dir)) {
		if(long_info) {
			if(dir_entry->f_type == FILE_DIRECTORY)
				printk("%4d  %12s  %s\r", COLOR_GREEN, dir_entry->inode_no, dir_entry->f_type == FILE_REGULAR ? "regular" : "directory", dir_entry->filename);
			else
				printk("%4d  %12s  %s\r", dir_entry->inode_no, dir_entry->f_type == FILE_REGULAR ? "regular" : "directory", dir_entry->filename);
		}
		else {
			if(dir_entry->f_type == FILE_DIRECTORY)
				printk("%s  ", COLOR_GREEN, dir_entry->filename);
			else
				printk("%s  ", dir_entry->filename);
		}
	}

	if(!long_info)
		printk("\r");

	sys_closedir(dir);
}

int32_t buildin_touch(uint32_t argc, char **argv) {
	int32_t ret = -1;
	if(argc != 2) {
		ERROR("touch only support 1 argument\r");
	} else {
		make_clear_abs_path(argv[1], final_path);
		if(strcmp("/", final_path)) {
			if(sys_open(final_path, O_CREAT) == 0)
				ret = 0;
			else
				printk("touch : touch %s failed\r", final_path);
		}
	}
}

int32_t buildin_rm(uint32_t argc, char **argv) {
	int32_t ret = -1;
	if(argc != 2) {
		ERROR("rm only support 1 argument\r");
	} else {
		make_clear_abs_path(argv[1], final_path);
		if(strcmp("/", final_path)) {
			if(sys_unlink(final_path) == 0)
				ret = 0;
			else
				printk("rm : remove %s failed\r", final_path);
		}
	}
}

int32_t buildin_mkdir(uint32_t argc, char **argv) {
	int32_t ret = -1;
	if(argc != 2) {
		ERROR("mkdir only support 1 argument\r");
	} else {
		ret = make_clear_abs_path(argv[1], final_path);
		if(ret == -1) {
			ERROR("get clear abs path failed\r");
			return ret;
		}

		if(strcmp("/", final_path) != 0) {
			ret = sys_mkdir(final_path);
			if(ret != 0)
				printk("mkdir : create directory %s failed\r", final_path);
		}
	}

	return ret;
}

int32_t bildin_rmdir(uint32_t argc, char **argv) {
	int32_t ret = -1;
	if(argc != 2) {
		ERROR("rmdir only support 1 argument\r");
	} else {
		make_clear_abs_path(argv[1], final_path);
		if(strcmp("/", final_path)) {
			if(sys_rmdir(final_path) == 0)
				ret = 0;
			else
				printk("rmdir : remove %s failed\r", final_path);
		}
	}

	return ret;
}

char *buildin_cd(uint32_t argc, char **argv) {
	if(argc > 2) {
		ERROR("only support 1 argument\r");
		return NULL;
	}

	if(argc == 1)
		strcpy(final_path, "/");
	else
		make_clear_abs_path(argv[1], final_path);
	
	if(sys_chdir(final_path) == -1) {
		ERROR("no such directory\r");
		return NULL;
	}
	return final_path;
}

void buildin_pwd(uint32_t argc, char **argv) {
	if(argc != 1) {
		ERROR("no argument support\r");
		return;
	} else {
		if(NULL != sys_getcwd(final_path, MAX_PATH_LEN)) {
			printk("%s\r", final_path);
		} else {
			ERROR("get current work directory faild\r");
		}
	}
}