#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "../stdint.h"

typedef enum bitmap_state {
	bitmap_unused = 0,
	bitmap_used,
}bitmap_state_t;

/* 使用1位管理1页内存 */
typedef struct bitmap {
	uint32_t bitmap_byte_len; //bitmap用了多少字节
	uint8_t *map; //映射地址页 0:地址页没有被使用 1:地址页被使用
}bitmap_t;

extern void bitmap_init(bitmap_t *bitmap);
extern bitmap_state_t bitmap_scan(bitmap_t *bitmap, uint32_t index);
extern int bitmap_continuous_scan(bitmap_t *bitmap, uint32_t count);
extern void bitmap_set(bitmap_t *bitmap, uint32_t index, uint8_t value);

#endif