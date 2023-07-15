#include "../include/stdint.h"
#include "../include/kernel/thread.h"
#include "../include/kernel/fs.h"
#include "../include/string.h"
#include "../include/kernel/mm.h"
#include "../include/asm/system.h"

#include "../include/kernel/debug.h"

extern void interrupt_exit(void);
extern const unsigned char user_elf[];

typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

#define ET_EXEC 2 // 可执行文件
#define EM_386 3 // Intel 80386

#define ELFMAG0 0x7f	/* Magic number byte 0 */
#define ELFMAG1 'E'		/* Magic number byte 1 */
#define ELFMAG2 'L'		/* Magic number byte 2 */
#define ELFMAG3 'F'		/* Magic number byte 3 */
#define ELFCLASS32 1	/* 32位ELF文件 */
#define ELFDATA2LSB 1	/* 小端字节序 Least Significant Byte */
#define EV_CURRENT 1	/* 当前的ELF文件版本 */
const static unsigned char magic[16] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS32, ELFDATA2LSB, EV_CURRENT};

/*
 @brief 文件头 ELF文件的基本信息的数据结构
 ELF(Executable and Linkable Format)
 Ehdr(ELF Header)
 */
struct Elf32_Ehdr {
	unsigned char	e_ident[16];	// 0 ELF 标识符
	Elf32_Half		e_type;			// 16 文件类型
	Elf32_Half		e_machine;		// 18 目标体系结构
	Elf32_Word		e_version;		// 20 文件版本
	Elf32_Addr		e_entry;		// 24 程序入口点的虚拟地址
	Elf32_Off		e_phoff;		// 28 程序头表的文件偏移
	Elf32_Off		e_shoff;		// 32 节头表的文件偏移
	Elf32_Word		e_flags;		// 36 处理器特定标志
	Elf32_Half		e_ehsize;		// 40 文件头的大小
	Elf32_Half		e_phentsize;	// 42 程序头表中每个条目的大小
	Elf32_Half		e_Phnum;		// 44 程序头表中的条目数
	Elf32_Half		e_shentsize;	// 46 节头表中每个条目的大小
	Elf32_Half		e_shnum;		// 48 节头表中的条目数
	Elf32_Half		e_shstrndx;		// 50 节头表字符串表的索引
};

/*
 @brief 程序头表 描述ELF文件中的一个程序头表条目
 Phdr(Program Header)
 */
struct Elf32_Phdr {
	Elf32_Word	p_type;		// 0 段类型
	Elf32_Off	p_offset;	// 4 段在文件中的偏移量
	Elf32_Addr	p_vaddr;	// 8 段的虚拟地址
	Elf32_Addr	p_paddr;	// 12 段的物理地址
	Elf32_Word	p_filesz;	// 16 段在文件中的大小
	Elf32_Word	p_memsz;	// 20 段在内存中的大小
	Elf32_Word	p_flags;	// 24 段的标志
	Elf32_Word	p_align;	// 28 段在内存中的对齐方式
};

enum segment_type {
	PT_NULL,		// 0
	PT_LOAD,		// 1 可加载段
	PT_DYNAMIC,		// 2 动态链接信息段
	PT_INTERP,		// 3 解释器路径段
	PT_NOTE,		// 4 注释段
	PT_SHLIB,		// 5 保留
	PT_PHDR			// 6 程序头表段
};

static bool segment_load(int32_t fd, uint32_t offset, uint32_t filesz, uint32_t vaddr) {
	uint32_t vaddr_first_page = vaddr & 0xFFFFF000;
	uint32_t size_in_first_page = PAGE_SIZE - (vaddr & 0xFFF);
	uint32_t occupy_pages = 0;
	if(filesz > size_in_first_page) {
		uint32_t left_size = filesz - size_in_first_page;
		occupy_pages = DIV_ROUND_UP(left_size, PAGE_SIZE) + 1;
	} else
		occupy_pages = 1;

	uint32_t page_index = 0;
	uint32_t vaddr_page = vaddr_first_page;
	while(page_index < occupy_pages) { // 进程分配内存
		uint32_t *pde = get_pde_ptr(vaddr_page);
		uint32_t *pte = get_pte_ptr(vaddr_page);

		if(!(*pde & 0x1) || !(*pte & 0x1)) { // p位 不存在分配内存
			if(malloc_a_page(pf_user, vaddr_page) == NULL)
				return false;
		}

		vaddr_page += PAGE_SIZE;
		page_index++;
	}
	sys_lseek(fd, offset, SEEK_SET);
	uint32_t size = sys_read(fd, vaddr, filesz); // 拷贝到内存
	if(size != filesz) {
		ERROR("sys_read read elf error\r");
		return false;
	}
	if(filesz > 0) {
		if(memcmp((void *)vaddr, user_elf + offset, filesz)) {
			ERROR("cmp elf error\r");
			return false;
		}
	}

	return true;
}

static int32_t load(const char *pathname) {
	int32_t ret = -1;
	struct Elf32_Ehdr elf_header = {0};
	struct Elf32_Phdr prog_header = {0};

	int32_t fd = sys_open(pathname, O_RDONLY);
	if(fd == -1)
		return -1;
	
	if(sys_read(fd, &elf_header, sizeof(struct Elf32_Ehdr)) != sizeof(struct Elf32_Ehdr)) {
		return -1;
		goto done;
	}

	if(strcmp(elf_header.e_ident, magic) \
	|| elf_header.e_type != ET_EXEC \
	|| elf_header.e_machine != EM_386 \
	|| elf_header.e_version != EV_CURRENT \
	|| elf_header.e_Phnum > 1024 \
	|| elf_header.e_phentsize != sizeof(struct Elf32_Phdr)) {
		ret = -1;
		goto done;
	}

	Elf32_Half prog_header_size = elf_header.e_phentsize;
	Elf32_Off prog_header_offset = elf_header.e_phoff;
	uint32_t prog_index = 0;
	while(prog_index < elf_header.e_Phnum) { // 遍历程序头
		memset(&prog_header, 0, prog_header_size);

		sys_lseek(fd, prog_header_offset, SEEK_SET); // 1.定位程序头

		if(sys_read(fd, &prog_header, prog_header_size) != prog_header_size) { // 2.读取程序头
			ret = -1;
			goto done;
		}

		// INFO("addr : %#x\r", prog_header.p_vaddr);
		// INFO("offset : %#x\r", prog_header.p_offset);
		// INFO("size : %#x\r", prog_header.p_filesz);
		
		if(PT_LOAD == prog_header.p_type) { // 3.加载程序到内存
			if(!segment_load(fd, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr)) {
				ret = -1;
				goto done;
			}
		}

		prog_header_offset += elf_header.e_phentsize; // 4.下一个程序头
		prog_index++;
	}
	ret = elf_header.e_entry;

done:
	sys_close(fd);
	return ret;
}

int32_t sys_execv(const char *path, const char *argv[]) {
	uint32_t argc = 0;
	// while(argv[argc]) // debug
	// 	argc++;

	int32_t entry_point = load(path);
	if(entry_point == -1)
		return -1;

	task_t *current = running_thread();
	memcpy(current->name, path, 32);

	interrupt_stack_t *int_0_stack = (interrupt_stack_t *)((uint32_t)current + PAGE_SIZE - sizeof(interrupt_stack_t));
	// int_0_stack->ebx = (int32_t)argv;
	// int_0_stack->ecx = argc;

	int_0_stack->edi = int_0_stack->esi = int_0_stack->ebp = int_0_stack->esp = 0;
	int_0_stack->ebx = int_0_stack->edx = int_0_stack->ecx = int_0_stack->eax = 0;
	int_0_stack->gs = int_0_stack->fs = int_0_stack->es = int_0_stack->ds = R3_DATA_SELECTOR;
	int_0_stack->eip = (uint32_t)entry_point;
	int_0_stack->cs = R3_CODE_SELECTOR;
	int_0_stack->eflags = EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1;
	int_0_stack->esp_old = 0xC0000000;
	int_0_stack->ss_old = R3_DATA_SELECTOR;

	BOCHS_DEBUG_MAGIC
	BOCHS_DEBUG_MAGIC

	__asm__("mov esp, %0;"
		"pop gs;"
		"pop fs;"
		"pop es;"
		"pop ds;"
		"popad;"
		"add esp, 8;"
		"iret"
		:: "g" (int_0_stack): "memory");
}