#include "../include/stdlib.h"
#include "../include/unistd.h"

void *malloc(size_t size) {
	void *ptr = NULL;
	__asm__ volatile("int 0x80"
		: "=a" (ptr)
		: "0" (__NR_malloc), "b" (size)
	);

	return ptr;
}

void free(void *addr, size_t size) {
	__asm__ volatile("int 0x80"
		:: "a" (__NR_free), "b" (addr), "c" (size)
	);

	return;
}