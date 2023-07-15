#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "../stdint.h"

extern void *sys_malloc(size_t size);
extern void sys_free(void *addr, size_t size);
extern pid_t sys_get_pid(void);
extern pid_t sys_get_ppid(void);
extern pid_t sys_fork(void);
extern int32_t sys_execv(const char *path, const char *argv[]);

#endif