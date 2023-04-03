;setup被mbr加载到内存0x500处
;mbr jmp 0x500 跳到setup

%include "boot.inc"

[BITS 16]
[ORG SETUP_ADDR]

SECTION setup
	mov byte [gs:0x0], '2'
	mov byte [gs:0x1], 0xA4

	mov byte [gs:0x2], ' '
	mov byte [gs:0x3], 0xA4

	mov byte [gs:0x4], 'S'
	mov byte [gs:0x5], 0xA4

	mov byte [gs:0x6], 'E'
	mov byte [gs:0x7], 0xA4

	mov byte [gs:0x8], 'T'
	mov byte [gs:0x9], 0xA4

	mov byte [gs:0xA], 'U'
	mov byte [gs:0xB], 0xA4

	mov byte [gs:0xC], 'P'
	mov byte [gs:0xD], 0xA4

;内存检测
memory_check:
	xor ebx, ebx ;首次调用ebx = 0
	mov di, ARDS_BUFFER ;ARDS存放地址
	mov word [ARDS_TIMES_BUFFER], 0 ;检测次数,每次返回一个ARDS

.loop:
	mov eax, 0xE820
	mov ecx, 20
	mov edx, 0x534D4150
	int 0x15
	
	jc memory_check_error

	add di, cx
	inc word [ARDS_TIMES_BUFFER]

	cmp ebx, 0 ;bios写数据判断是否结束内存检测
	jne .loop

	;进入保护模式 构建GDT表,打开A20总线,cr0 PE位置一
	xchg bx, bx

	;关中断
	cli
	
	lgdt [gdt_ptr]

	in al, 0x92
	or al, 0b0000_0010
	out 0x92, al

	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax
	
	jmp R0_CODE_SELECTOR:protected_mode
	
	jmp $

[BITS 32]
protected_mode:
	xchg bx, bx

	mov ax, R0_DATA_SELECTOR
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ax, VIDEO_MEM_SELECTOR
	mov gs, ax
	mov esp, 0x9F000

	mov byte [gs:160], 'p'

	;开启分页 构建页目录表gdt和页表gtt 页目录表地址写入cr3 cr0的pg位置1
	call setup_page

	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	sgdt [gdt_ptr]

	;开启分页后 操作的都是虚拟地址 访问gdt,显存段,esp都使用0xC0000000以上的地址
	mov ebx, [gdt_ptr + 2]
	or dword [ebx + 8 * 3 + 4], 0xC0000000 ;baseaddr 24-31

	add dword [gdt_ptr + 2], 0xC0000000

	add esp, 0xC0000000

	lgdt [gdt_ptr]

	mov byte [gs:160], 'v'

	jmp $


memory_check_error:
	mov byte [gs:160], 'm'
	mov byte [gs:162], 'e'
	mov byte [gs:164], 'r'
	mov byte [gs:168], 'r'

	jmp $

setup_page:
;清零
	mov ecx, 0x400
	mov esi, 0
.clear_pdt:
	mov dword [PAGE_DIR_TABLE_POS + esi * 4], 0
	inc esi
	loop .clear_pdt

.create_ptt:
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000

	or eax, PAGE_US_U | PAGE_RW_W | PAGE_P
	mov [PAGE_DIR_TABLE_POS + 0x0], eax ;0 - 0x40000
	mov [PAGE_DIR_TABLE_POS + 0xC00], eax ;映射到0xC0000000以上,仿linux操作系统内存空间 0xC0000000 - 0xC0040000
	
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 0xFFC], eax ;最后一个页表存放页目录表本身 用于虚拟内存访问页目录表,页表

;挂载0 - 1M物理内存
	mov ebx, PAGE_DIR_TABLE_POS
	add ebx, 0x1000
	mov ecx, 0x100
	mov esi, 0
	mov edx, PAGE_US_U | PAGE_RW_W | PAGE_P
.create_pte:
	mov [ebx + esi * 4], edx
	add edx, 0x1000
	inc esi
	loop .create_pte

;先创建0xC0040000 - 0xFFFFFFFF的页表
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000
	or eax, PAGE_US_U | PAGE_RW_W | PAGE_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 0xFE
	mov esi, 0xC00 >> 2 + 1
.create_kernel_ptt:
	mov [ebx + esi * 4], eax
	inc esi
	add eax, 0x1000
	loop .create_kernel_ptt

	ret

SECTION gdt
gdt_base:
	dd 0, 0
gdt_code:
	dw R0_CODE_LIMIT & 0xFFFF
	dw R0_CODE_BASEADDR & 0xFFFF
	db R0_CODE_BASEADDR >> 16 & 0xFF
	db GDT_P | GDT_DPL_0 | GDT_S_CODE_DATA | GDT_TYPE_CODE
	db GDT_G_4K | GDT_D_B_32 | GDT_L_32 | GDT_AVL | (R0_CODE_LIMIT >> 16 & 0xF)
	db R0_CODE_BASEADDR >> 24 & 0xFF
gdt_data:
	dw R0_DATA_LIMIT & 0xFFFF
	dW R0_DATA_BASEADDR & 0xFFFF
	db R0_DATA_BASEADDR >> 16 & 0xFF
	db GDT_P | GDT_DPL_0 | GDT_S_CODE_DATA | GDT_TYPE_DATA
	db GDT_G_4K | GDT_D_B_32 | GDT_L_32 | GDT_AVL | (R0_DATA_LIMIT >> 16 & 0xF)
	db R0_DATA_BASEADDR >> 24 & 0xFF
gdt_video_memory:
	dw VIDEO_MEM_LIMIT & 0xFFFF
	dw VIDEO_MEM_BASEADDR & 0xFFFF
	db VIDEO_MEM_BASEADDR >> 16 & 0xFF
	db 0b1_00_1_0010
	db GDT_G_1B | GDT_D_B_32 | GDT_L_32 | GDT_AVL | (VIDEO_MEM_LIMIT >> 16 & 0xF)
	db VIDEO_MEM_BASEADDR >> 24 & 0xFF
gdt_ptr:
	dw $ - gdt_base
	dd gdt_base
