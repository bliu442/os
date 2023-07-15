#ifndef __UNISTD_H__
#define __UNISTD_H__

#include "./stdint.h"

extern int errno;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define __NR_malloc 0
#define __NR_free 1
#define __NR_get_pid 2
#define __NR_get_ppid 3
#define __NR_fork 4
#define __NR_execv 5

#define __NR_write 20
#define __NR_read 21
#define __NR_lseek 22
#define __NR_open 23
#define __NR_close 24

#define __NR_unlink 25
#define __NR_stat 26

extern pid_t get_pid(void);
extern pid_t get_ppid(void);
extern pid_t fork(void);
extern int32_t execv(const char * path, const char ** argv);

#define _syscall0(type, name) \
type name(void) \
{ \
	long __res; \
	__asm__ volatile("int 0x80" \
		: "=a" (__res) \
		: "0" (__NR_##name) \
	); \
	if(__res >= 0) \
		return (type) __res; \
	errno = -__res; \
	return -1; \
}

#define _syscall1(type, name, atype, a) \
type name(atype a) \
{ \
	long __res; \
	__asm__ volatile("int 0x80" \
		: "=a" (__res) \
		: "0" (__NR_##name), "b" ((long)(a)) \
	); \
	if(__res >= 0) \
		return (type) __res; \
	errno = -__res; \
	return -1; \
}

#define _syscall2(type, name, atype, a, btype, b) \
type name(atype a, btype b) \
{ \
	long __res; \
	__asm__ volatile("int 0x80" \
		: "=a" (__res) \
		: "0" (__NR_##name), "b" ((long)(a)), "c" ((long)(b)) \
	); \
	if(__res >= 0) \
		return __res; \
	errno = -__res; \
	return -1; \
}

#define _syscall3(type, name, atype, a, btype, b, ctype, c) \
type name(atype a, btype b, ctype c) \
{ \
	long __res; \
	__asm__ volatile("int 0x80" \
		: "=a" (__res) \
		: "0" (__NR_##name), "b" ((long)(a)), "c" ((long)(b)), "d" ((long)(c)) \
	); \
	if(__res >= 0) \
		return __res; \
	errno = -__res; \
	return __res; \
}

#endif