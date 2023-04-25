/* 用户态格式化打印函数printf */

#include "../include/kernel/print.h"
#include "../include/stdarg.h"
#include "../include/unistd.h"

static char buf[1024]; //mark

/*
 @brief 格式化打印
 @param fmt 字符串首地址
 @param ... 变参
 @retval 打印字符的个数
 */
int printf(const char *fmt, ...) {
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	write(STDOUT_FILENO, buf, i);

	return i;
}