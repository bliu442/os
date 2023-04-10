#ifndef __STRING_H__
#define __STRING_H__

#include "./stdint.h"

extern char *strcpy(char *dest, const char *src);
extern char *strcat(char *dest, const char *src);
extern size_t strlen(const char *str);
extern int strcmp(const char *lhs, const char *rhs);
extern char *strchr(const char *str, int ch);
extern char *strrchr(const char *str, int ch);
extern int memcmp(const void *lhs, const void *rhs, size_t count);
extern void *memset(void *dest, int ch, size_t count);
extern void *memcpy(void *dest, const void *src, size_t count);
extern void *memchr(const void *str, int ch, size_t count);

#endif
