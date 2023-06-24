#ifndef __BUILDIN_CMD_H__
#define __BUILDIN_CMD_H__

#include "../stdint.h"

extern void buildin_ls(uint32_t argc, char **argv);
extern int32_t buildin_mkdir(uint32_t argc, char **argv);

#endif