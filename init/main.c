#include "../include/kernel/print.h"
#include "../include/kernel/interrupt.h"
#include "../include/kernel/traps.h"
#include "../include/asm/system.h"
#include "../include/asm/io.h"
#include "../include/string.h"
#include "../include/kernel/mm.h"
#include "../include/kernel/thread.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/process.h"
#include "../include/unistd.h"
#include "../include/kernel/syscall.h"
#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/kernel/debug.h"
#include "../include/kernel/hd.h"
#include "../include/kernel/shell.h"
#include "../include/kernel/fs.h"
#include "../include/kernel/dir.h"
#include "../include/kernel/file.h"
#include "../user_program/user.elf.h"

#include "../include/kernel/debug.h"

extern void u_process_a(void);

void _start(void) {
	put_str("\rkernel!\r");
	pic_init();
	idt_init();
	gdt_init();
	clock_init();

	print_check_memory_info();
	physics_memory_init();
	physics_memory_pool_init();

	virtual_memory_init();
	virtual_memory_pool_init();

	pthread_init();

	console_init(); //之后使用C语言写的显卡驱动
	printk_init();

	shell_init();

	STI
	out_byte(PIC_M_DATA, 0b11111000); //打开0时钟中断 1键盘中断 2号级联
	out_byte(PIC_S_DATA, 0b00111111); //打开两个硬盘中断 14 15

	hd_init();
	file_system_init();
	open_root_dir(current_part);

	sys_open("/file1", O_CREAT);
	sys_open("/file2", O_CREAT);
	sys_mkdir("/dir1");
	sys_mkdir("/dir2");

#if 0
	uint8_t buf[32] = {0};
	uint32_t fd = sys_open("/file1", O_RDWR);
	sys_write(fd, "hello world\r", sizeof("hello world\r"));
	sys_lseek(fd, 0, SEEK_SET);
	sys_read(fd, buf, sizeof(buf));
	PRINT_HEX(buf, sizeof(buf));
	INFO(buf, strlen(buf));
	sys_close(fd);
#endif

#if 0 // 将elf文件写入磁盘
	sys_open("/user_program", O_CREAT);
	uint32_t fd1 = sys_open("/user_program", O_RDWR);
	sys_write(fd1, user_elf, user_elf_len);
	sys_close(fd1);
#endif
#if 0 // 读校验
	uint8_t *elf_buf = kmalloc(512);
	if(elf_buf == NULL) {
		ERROR("kmalloc\r");
	}
	uint32_t fd1 = sys_open("/user_program", O_RDONLY); // 能确保磁盘中的数据是正确的,但读磁盘过快会失败
	uint32_t offset = 0;
	uint32_t read_size = 0;
	uint32_t index = 0;
	while(offset < user_elf_len) {
		memset(elf_buf, 0, 512);
		read_size = sys_read(fd1, elf_buf, 512);
		if(memcmp(elf_buf, user_elf + offset, read_size)) {
			ERROR("read disk error\r");
		}
		offset += read_size;
		index++;
	}
	sys_close(fd1);
#endif

	thread_start("shell", 31, shell, NULL);
	process_start(u_process_a, "u_process_a");

	printk("main pid : %#x\r", sys_get_pid());
	while(1);
}

void u_process_a(void) {
	pid_t pid = get_pid();

	printf("u_process_a pid : %#x\r", pid);

	pid_t child_pid = fork();
	if(child_pid != 0) {
		INFO("parent\r");
		INFO("pid : %d, child pid : %d\r", get_pid(), child_pid);
	} else {
		INFO("child\r");
		INFO("pid : %d, parent pid : %d\r", get_pid(), get_ppid());
	
#if 1
		stat_t file_stat = {0};
		if(stat("/user_program", &file_stat) == -1) {
			ERROR("cannot access %s No such file or firectory\r", "/user_program");
		} else {
			execv("/user_program", NULL);
		}
#endif

	}
	
	INFO("two print\r");

	while(1);
}