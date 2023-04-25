#include "../include/unistd.h"

pid_t get_pid(void) {
	pid_t pid = 0;

	__asm__ volatile("int 0x80"
		: "=a" (pid)
		: "0" (__NR_get_pid)
	);

	return pid;
}