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

	jmp $


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
