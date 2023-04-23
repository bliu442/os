#include "../include/unistd.h"

_syscall3(int, write, int, fd, const char *, buf, int, count)

#if 0
#define __NR_write 0

设置寄存器
edx = count
ecx = buf
ebx = fd
eax = __NR_write
int 0x80触发软中断
int write(int fd, const char *buf, int count) {
	long __res;
	__asm__ volatile("int 0x80"
		: "=a" (__res);
		: "0" (__NR_write), "b" ((long)(a)), "c" ((long)(b)), "d" ((long)(c));
	);
	return __res;
}
#endif