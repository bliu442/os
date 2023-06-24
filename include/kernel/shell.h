#ifndef __SHELL_H__
#define __SHELL_H__

#include "../stdint.h"

#define cmd_size 128
#define MAX_ARG_NUMBER 16

extern void shell_init(void);
extern void shell(void *arg);

#endif