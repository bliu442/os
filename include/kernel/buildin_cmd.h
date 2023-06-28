#ifndef __BUILDIN_CMD_H__
#define __BUILDIN_CMD_H__

#include "../stdint.h"

extern char final_path[512];

extern void buildin_ls(uint32_t argc, char **argv);
extern int32_t buildin_touch(uint32_t argc, char **argv);
extern int32_t buildin_rm(uint32_t argc, char **argv);
extern int32_t buildin_mkdir(uint32_t argc, char **argv);
extern int32_t bildin_rmdir(uint32_t argc, char **argv);
extern char *buildin_cd(uint32_t argc, char **argv);
extern void buildin_pwd(uint32_t argc, char **argv);

#endif