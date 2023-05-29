#ifndef __PRINT_H__
#define __PRINT_H__

#include "../stdint.h"
#include "../stdarg.h"

#define COLOR_MAGIC 0x5B3B5D00

#define FORECOLOR_BLACK 0x0
#define FORECOLOR_BLUE 0x1
#define FORECOLOR_GREEN 0x2
#define FORECOLOR_CYAN 0x3
#define FORECOLOR_RED 0x4
#define FORECOLOR_MAGENTA 0x5
#define FORECOLOR_BROWN 0x6
#define FORECOLOR_WHITE 0x7
#define BRIGHTNESS 0x8
#define FORECOLOR_YELLOW (FORECOLOR_BROWN | BRIGHTNESS)

#define BACKCOLOR_BLACK 0x00
#define BACKCOLOR_BLUE 0x10
#define BACKCOLOR_GREEN 0x20
#define BACKCOLOR_CYAN 0x30
#define BACKCOLOR_RED 0x40
#define BACKCOLOR_MAGENTA 0x50
#define BACKCOLOR_BROWN 0x60
#define BACKCOLOR_WHITE 0x70
#define FLICKER 0x80

#define WHITE (COLOR_MAGIC | FORECOLOR_WHITE | BACKCOLOR_BLACK)
#define RED (COLOR_MAGIC | FORECOLOR_RED | BACKCOLOR_BLACK)
#define YELLOW (COLOR_MAGIC | FORECOLOR_YELLOW | BACKCOLOR_BLACK)
#define GREEN (COLOR_MAGIC | FORECOLOR_GREEN | BACKCOLOR_BLACK)

extern void put_char(uint8_t char_ascii);
extern void put_str(uint8_t *str);
extern void put_int(uint32_t num);

extern void console_init(void);

extern void console_put_char(char ch);
extern void console_put_str(char *str);
extern void console_put_int(uint32_t num);

extern int console_write(char *buf, uint32_t count, ...);
extern int vsprintf(char *buf, const char *fmt, va_list args);

extern void printk_init(void);
extern int printk(const char *fmt, ...);

extern int printf(const char *fmt, ...);

#endif