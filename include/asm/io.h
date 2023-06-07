#ifndef __IO_H__
#define __IO_H__

/* see io.asm */

extern char in_byte(int port);
extern void out_byte(int port, int value);
extern short in_word(int port);
extern void out_word(int port, int value);

#define port_read(port, buf, nr) __asm__("cld; rep; insw" :: "d" (port), "D" (buf), "c" (nr))
#define port_write(port, buf, nr) __asm__("cld; rep; outsw" :: "d" (port), "S" (buf), "c" (nr))

#endif