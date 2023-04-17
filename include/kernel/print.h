#ifndef __PRINT_H__
#define __PRINT_H__

#include "../stdint.h"

extern void put_char(uint8_t char_ascii);
extern void put_str(uint8_t *str);
extern void put_int(uint32_t num);

extern void console_init(void);
extern void console_write(char *buf, uint32_t count);

extern void console_put_char(char ch);
extern void console_put_str(char *str);
extern void console_put_int(uint32_t num);

#endif