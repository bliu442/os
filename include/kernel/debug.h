#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "../stdint.h"
#include "./print.h"
#include "./global.h"

#define DEBUG

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef DEBUG
#define ASSERT(CONDITION) if(CONDITION) {} else {PANIC(#CONDITION);}
#else
#define ASSERT(CONDITION) ((void)0)
#endif

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 3
#endif

#ifndef TAG
#define TAG "debug"
#endif

#define DEBUG_COLOR_ENABLR 1
#if DEBUG_COLOR_ENABLR
#define COLOR_RED RED
#define COLOR_YELLOW YELLOW
#define COLOR_GREEN GREEN
#else
#define COLOR_RED
#define COLOR_YELLOW
#define COLOR_GREEN
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

extern void panic_spin(char *filename, int line, const char *function, const char *condition);

#endif