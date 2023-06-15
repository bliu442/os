/* 调试 */

#include "../include/kernel/print.h"
#include "../include/asm/system.h"
#include "../include/kernel/debug.h"

#if 0
/* @brief 打印错误信息 */
void panic_spin(uint8_t *filename, int line, uint8_t *function, uint8_t *condition) {
	CLI;

	put_str("\r\n*************error!!!*************\r\n");
	put_str("filename : ");
	put_str(filename);
	put_char('\r');
	put_str("line : ");
	put_int(line);
	put_char('\r');
	put_str("funcname : ");
	put_str(function);
	put_char('\r');
	put_str("condition : ");
	put_str(condition);
	put_char('\r');
	while(1);
}
#else
void panic_spin(uint8_t *filename, int line, uint8_t *function, uint8_t *condition) {
	CLI;

	printk("\r*************error!!!*************\r");
	printk("filename : %s\r", filename);
	printk("line : %d\r", line);
	printk("function : %s\r", function);
	printk("condition : %s\r", condition);
	while(1);
}
#endif

void printk_hex(void *buf, uint32_t len) {
	uint8_t *ptr = buf;

	if(len > 0) {
		for(uint32_t i = 1;i <= len;++i) {
			if((i & 0xF) == 1) {
				if(i != 1)
					printk("\r");
				printk("[HEX]: ", COLOR_GREEN);
			}
			printk("%#2x ", *ptr++);
		}
		printk("\r");
	}
}

void printf_hex(void *buf, uint32_t len) {
	uint8_t *ptr = buf;

	if(len > 0) {
		for(uint32_t i = 1;i <= len;++i) {
			if((i & 0xF) == 1) {
				if(i != 1)
					printf("\r");
				printf("[HEX]: ");
			}
			printf("%#2x ", *ptr++);
		}
		printf("\r");
	}
}