void kernel_main(void) {
	__asm__("xchg bx, bx");

	char *ptr = (char *)(0xB8000 + 160);
	*ptr = 'c';

	while(1);
}