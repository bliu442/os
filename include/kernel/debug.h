#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "../stdint.h"
#include "./print.h"
#include "./global.h"

#define DEBUG

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))
#define list_offset(struct_type, member) (int)(&((struct_type *)0)->member) //结构体成员member在结构体中的偏移量
#define item2entry(struct_type, struct_member_name, item_ptr) (struct_type *)((int)item_ptr - list_offset(struct_type, struct_member_name)) //根据结构体成员地址找到结构体起始地址

#ifdef DEBUG
#define ASSERT(CONDITION) if(CONDITION) {} else {PANIC(#CONDITION);}
#else
#define ASSERT(CONDITION) ((void)0)
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

#ifndef TAG
#define TAG "debug"
#endif

#define DEBUG_COLOR_ENABLR 1
#if DEBUG_COLOR_ENABLR
#define COLOR_RED RED
#define COLOR_YELLOW YELLOW
#define COLOR_GREEN GREEN
#define COLOR_BLUE BLUE
#define COLOR_WATHET WATHET
#else
#define COLOR_RED
#define COLOR_YELLOW
#define COLOR_GREEN
#define COLOR_BLUE
#define COLOR_WATHET
#endif

#if (DEBUG_LEVEL > 0)
#define ERROR(format, ...) do { \
	uint32_t cs_value = 0; \
	GET_CS(cs_value); \
	if((cs_value & CS_RPL) == SELECTOR_RPL_3) { \
		printf("[error] %s:%s:%d : ", TAG, __func__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} else { \
		printk("[error] %s:%s:%d : ", COLOR_RED, TAG, __func__, __LINE__); \
		printk(format, COLOR_RED, ##__VA_ARGS__); \
	} \
} while(0);
#else
#define ERROR(format, ...)
#endif

#if (DEBUG_LEVEL > 1)
#define WARN(format, ...) do { \
	uint32_t cs_value = 0; \
	GET_CS(cs_value); \
	if((cs_value & CS_RPL) == SELECTOR_RPL_3) { \
		printf("[warn] %s:%s:%d : ", TAG, __func__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} else { \
		printk("[warn] %s:%s:%d : ", COLOR_YELLOW, TAG, __func__, __LINE__); \
		printk(format, COLOR_YELLOW, ##__VA_ARGS__); \
	} \
} while(0);
#else
#define WARN(format, ...)
#endif

#if (DEBUG_LEVEL > 2)
#define INFO(format, ...) do { \
	uint32_t cs_value = 0; \
	GET_CS(cs_value); \
	if((cs_value & CS_RPL) == SELECTOR_RPL_3) { \
		printf("[info] %s:%s:%d : ", TAG, __func__, __LINE__); \
		printf(format, ##__VA_ARGS__); \
	} else { \
		printk("[info] %s:%s:%d : ", COLOR_GREEN, TAG, __func__, __LINE__); \
		printk(format, COLOR_GREEN, ##__VA_ARGS__); \
	} \
} while(0);
#else
#define INFO(format, ...)
#endif

#if (DEBUG_LEVEL > 3)
#define PRINT_HEX(buf, len) do { \
	uint32_t cs_value = 0; \
	GET_CS(cs_value); \
	if((cs_value & CS_RPL) == SELECTOR_RPL_3) { \
		printf_hex(buf, len); \
	} else { \
		printk_hex(buf, len); \
	} \
} while(0);
#else
#define PRINT_HEX(buf, len)
#endif

extern void panic_spin(uint8_t *filename, int line, uint8_t *function, uint8_t *condition);
extern void printk_hex(void *buf, uint32_t len);
extern void printf_hex(void *buf, uint32_t len);

#endif