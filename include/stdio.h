#ifndef __STDIO_H__
#define __STDIO_H__

#include "./stdint.h"
typedef struct stat stat_t;

extern int printf(const char *fmt, ...);

extern int write(int fd, const char *buf, int count);
extern ssize_t read(int32_t fd, void *buf, uint32_t count);
extern int32_t lseek(int32_t fd, int32_t offset, uint8_t whence);
extern int32_t open(const char *pathname, uint8_t flag);
extern int32_t close(int32_t fd);
extern int32_t unlink(const char *pathname);
extern int32_t stat(const char *path, stat_t *stat);

#endif