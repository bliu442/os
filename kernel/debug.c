/* 调试 */

#include "../include/kernel/print.h"
#include "../include/asm/system.h"

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