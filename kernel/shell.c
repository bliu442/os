#include "../include/kernel/shell.h"
#include "../include/kernel/ioqueue.h"
#include "../include/string.h"

#include "../include/kernel/debug.h"

extern ioqueue_t keyboard_buf;

static char cmd_line[cmd_size];

static void print_prompt(void) {
	printk("liuben@shell:", COLOR_BLUE);
	printk("pwd", COLOR_WATHET);
	printk("$ ");
}

void shell_init(void) {
	ioqueue_init(&keyboard_buf);
}

static void read_cmd(char *buf, int32_t count) {
	//1.从keyboard_buf读数据到cmd_line
	//2.读到\n 该条命令结束
	char *pos = buf;
	while(pos - buf < count) {
		*pos = ioqueue_getchar(&keyboard_buf);

		printk("%c", *pos);
		switch(*pos) {
			case '\n': //enter 命令结束
				*pos = '\0';
				return;
			case '\b': //backspace 删除一个字符
				if(cmd_line[0] != '\b')
					--pos;
				break;
		}

		pos++; //其余字符添加进cmd_line
	}
}

static cmd_execute(uint8_t *buf) {
	if(!strcmp("ls", buf)) {
		buildin_ls();
	}
}

void shell(void *arg) {
	while(1) {
		print_prompt();
		memset(cmd_line, 0, sizeof(cmd_line));
		read_cmd(cmd_line, sizeof(cmd_line));
	
		// cmd解析
		// PRINT_HEX(cmd_line, strlen(cmd_line));
		// INFO("%s\r", cmd_line);
		cmd_execute(cmd_line);
	}
}