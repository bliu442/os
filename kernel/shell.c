#include "../include/kernel/shell.h"
#include "../include/kernel/ioqueue.h"
#include "../include/string.h"
#include "../include/kernel/buildin_cmd.h"
#include "../include/kernel/fs.h"
#include "../include/unistd.h"

#define DEBUG_LEVEL 2
#include "../include/kernel/debug.h"

extern ioqueue_t keyboard_buf;

static char cmd_line[cmd_size];
int32_t argc = -1;
char *argv[MAX_ARG_NUMBER];

char cwd[MAX_PATH_LEN];

static void print_prompt(void) {
	printk("liuben@shell:", COLOR_BLUE);
	printk("%s", COLOR_WATHET, cwd);
	printk("$ ");
}

void shell_init(void) {
	ioqueue_init(&keyboard_buf);
}

static void read_cmd(char *buf, int32_t count) {
	//1.从keyboard_buf读数据到cmd_line
	//2.读到\n 该条命令结束
	char *pos = buf;
	while(sys_read(STDIN_FILENO, pos, 1) && pos - buf < count) {
		printk("%c", *pos);
		switch(*pos) {
			case '\n': //enter 命令结束
				*pos = '\0';
				return;
			case '\b': //backspace 删除一个字符
				if(cmd_line[0] != '\b')
					--pos;
				break;

			default:
				pos++; //其余字符添加进cmd_line
		}

	}
}

/*
 @brief 解析命令字符串
 @param cmd_str 命令字符串
 @param argv 参数指针数组
 @param token 分割符
 @retval 参数数量
 */
static int32_t cmd_parse(char *cmd_str, char **argv, char token) {
	int32_t arg_index = 0;
	while(arg_index < MAX_ARG_NUMBER) {
		argv[arg_index] = NULL;
		arg_index++;
	}

	char *next = cmd_str;
	int32_t argc = 0;
	while(*next) { // 遍历字符串
		while(*next == token) // 跳过分隔符,找到一个非分隔符的字符 命令
			next++;

		if(*next == '\0')
			break;
		
		argv[argc] = next;
		while(*next && *next != token) // 命令提取
			next++;
		
		if(*next) //命令分割
			*next++ = '\0';
		
		if(argc > MAX_ARG_NUMBER)
			return -1;
		
		argc++;
	}

	return argc;
}

static void cmd_execute(uint32_t argc, char **argv) {
	if(!strcmp("ls", argv[0])) {
		buildin_ls(argc, argv);
	} else if(!strcmp("mkdir", argv[0])) {
		buildin_mkdir(argc, argv);
	} else if(!strcmp("cd", argv[0])) {
		if(buildin_cd(argc, argv) != NULL) {
			memset(cwd, 0, MAX_PATH_LEN);
			strcpy(cwd, final_path);
		}
	} else if(!strcmp("pwd", argv[0])) {
		buildin_pwd(argc, argv);
	} else if(!strcmp("rmdir", argv[0])) {
		bildin_rmdir(argc, argv);
	} else if(!strcmp("rm", argv[0])) {
		buildin_rm(argc, argv);
	} else if(!strcmp("touch", argv[0])) {
		buildin_touch(argc, argv);
	}
}

void shell(void *arg) {
	cwd[0] = '/';
	while(1) {
		print_prompt();
		memset(cmd_line, 0, sizeof(cmd_line));
		read_cmd(cmd_line, sizeof(cmd_line));
	
		if(cmd_line[0] == '\0')
			continue;

		// cmd解析
		// PRINT_HEX(cmd_line, strlen(cmd_line));
		INFO("%s\r", cmd_line);
		argc = -1;
		argc = cmd_parse(cmd_line, argv, ' ');
		cmd_execute(argc, argv);
	}
}