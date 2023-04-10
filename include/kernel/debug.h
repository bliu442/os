#ifndef __DEBUG_H__
#define __DEBUG_H__

extern void panic_spin(char *filename, int line, const char *function, const char *condition);

#define DEBUG

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef DEBUG
#define ASSERT(CONDITION) if(CONDITION) {} else {PANIC(#CONDITION);}
#else
#define ASSERT(CONDITION) ((void)0)
#endif

#endif