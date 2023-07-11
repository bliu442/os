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

	i = write(STDOUT_FILENO, buf, i);

	return i;
}

_syscall3(ssize_t, read, int32_t, fd, void *, buf, uint32_t, count);
_syscall3(int32_t, lseek, int32_t, fd, int32_t, offset, uint8_t, whence);
_syscall2(int32_t, open, const char *, pathname, uint8_t, flag);
_syscall1(int32_t, close, int32_t, fd);
_syscall1(int32_t, unlink, const char *, pathname);
