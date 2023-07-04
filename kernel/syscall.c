/* 系统调用 */

#include "../include/kernel/syscall.h"
#include "../include/kernel/print.h"
#include "../include/kernel/thread.h"
#include "../include/kernel/fs.h"

#define SYSCALL_TABLE_SIZE 64

void *syscall_table[SYSCALL_TABLE_SIZE] = {
	sys_write,
	sys_get_pid,
	sys_malloc,
	sys_free,
	sys_fork,
};

pid_t sys_get_pid(void) {
	return thread_get_pid();
}

void *sys_malloc(size_t size) {
	return kmalloc(size);
}

void sys_free(void *addr, size_t size) {
	kfree(addr, size);
	return;
}