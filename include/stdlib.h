#ifndef __STDLIB_H__
#define __STDLIB_H__

#include "./stdint.h"

extern void *malloc(size_t size);
extern void free(void *addr, size_t size);

#endif