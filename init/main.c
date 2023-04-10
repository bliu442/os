#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"
#include "../include/asm/io.h"
#include "../include/string.h"

void _start(void) {
	put_str("\rkernel!\r");
	pic_init();
	idt_init();
	clock_init();

	char str1[10] = "hello";
	char str2[10] = "world";
	char str[20] = {0};

	strcpy(str, str1);
	strcat(str, str2);
	size_t len = strlen(str);
	int ret = strcmp(str, str1);
	char *ptr = strchr(str, 'w');
	ptr = NULL;
	ptr = strrchr(str, 'w');

	ret = memcmp(str1, str, 6);
	memset(str, 0, 5);
	memcpy(str, str1, 5);
	ptr = NULL;
	ptr = memchr(str, 'w', 10);

	STI
	out_byte(PIC_M_DATA, 0xFE); //打开时钟中断

	while(1);
}