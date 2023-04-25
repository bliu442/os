#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "../stdint.h"

extern ssize_t sys_write(int fd, const void *buf, size_t count);
extern pid_t sys_get_pid(void);

#endif