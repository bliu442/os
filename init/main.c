#include "../include/kernel/print.h"


void kernel_main(void) {
	put_char('\n');
	put_char('k');
	put_char('e');
	put_char('r');
	put_char('n');
	put_char('e');
	put_char('l');
	put_char('\r');

	put_str("kernel!\r");

	put_int(0x12345678);
	put_char('\r');
	put_int(0x00000000);

	char *ptr = "hello world!\r";
	console_init();
	console_write(ptr, 13);
	while(1);
}