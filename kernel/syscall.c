/* 系统调用 */

#include "../include/kernel/syscall.h"
#include "../include/kernel/print.h"
#include "../include/kernel/thread.h"

#define SYSCALL_TABLE_SIZE 64

void *syscall_table[SYSCALL_TABLE_SIZE] = {
	sys_write,
	sys_get_pid,
};

ssize_t sys_write(int fd, const void *buf, size_t count) {
	return console_write((char *)buf, count);
}

pid_t sys_get_pid(void) {
	return thread_get_pid();
}