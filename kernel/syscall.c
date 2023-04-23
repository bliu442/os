/* 系统调用 */

#include "../include/kernel/syscall.h"
#include "../include/kernel/print.h"

#define SYSCALL_TABLE_SIZE 64

void *syscall_table[SYSCALL_TABLE_SIZE] = {
	sys_write,
};

ssize_t sys_write(int fd, const void *buf, size_t count) {
	return console_write((char *)buf, count);
}