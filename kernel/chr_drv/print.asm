;简单的显存读写 以光标位置为基础 行25 列80,一页显示80 * 25 = 2000个字符,2000个字符占4000B,字符+颜色
;mark 固定屏幕显示起始字符的位置,只使用前4000B
;光标位置为字符的输入点坐标 人为约束,程序员维护,光标位置 * 2 = 显存中的地址

%include "../../boot/boot.inc"

CRT_ADDR_REG equ 0x3D4
CRT_DATA_REG equ 0x3D5

CRT_CURSOR_H equ 0xE ;光标的位置
CRT_CURSOR_L equ 0xF

WIDTH equ 80

ASCII_BS equ 0x08 ;\b
ASCII_LF equ 0x0A ;\n
ASCII_CR equ 0x0D ;\r

[bits 32]

section .text
global put_char
put_char:
	pushad ;保存通用寄存器

	mov ax, VIDEO_MEM_SELECTOR
	mov gs, ax

;读取光标位置 bx = 光标位置
.get_cursor:
	mov dx, CRT_ADDR_REG
	mov al, CRT_CURSOR_H
	out dx, al
	mov dx, CRT_DATA_REG
	in al, dx
	mov ah, al

	mov dx, CRT_ADDR_REG
	mov al, CRT_CURSOR_L
	out dx, al
	mov dx, CRT_DATA_REG
	in al, dx
	mov bx, ax

;拿到字符ASCII 8个通用寄存器+retaddr
	mov ecx, [esp + 9 * 4]

;特殊字符处理
	cmp cl, ASCII_CR
	je .is_cr
	cmp cl, ASCII_LF
	je .is_lf
	cmp cl, ASCII_BS
	je .is_bs
	
	jmp .put_other

.is_cr:
.is_lf:
	mov ax, bx
	mov si, WIDTH
	xor dx, dx ;余数
	div si
	sub bx, dx

	add bx, WIDTH
	cmp bx, 2000
	jl .set_cursor
	jmp .scroll_up

.is_bs:
	dec bx
	shl bx, 1

	mov byte [gs:bx], 0x20
	inc bx
	mov byte [gs:bx], 0x7
	shr bx, 1
	jmp .set_cursor

.put_other:
	shl bx, 1

	mov [gs:bx], cl
	inc bx
	mov byte [gs:bx], 0x7
	shr bx, 1
	inc bx
	cmp bx, 2000 ;只使用前4000B
	jl .set_cursor

;向上滚动一行
.scroll_up:
	cld
	mov ecx, 960 ;(2000 - 80) / 4 复制次数

	mov esi, 0xC00B80A0
	mov edi, 0xC00B8000
	rep movsd

	mov ebx, 3840 ;(2000 - 80) * 2 最后一行起始地址
	mov ecx, WIDTH
.clean:
	mov word [gs:ebx], 0x0720 ;黑底白字空格
	add ebx, 2
	loop .clean
	mov bx, 1920 ;(2000 - 80) 光标位置

;设置光标位置 bx = 光标位置
.set_cursor:
	mov dx, CRT_ADDR_REG
	mov al, CRT_CURSOR_H
	out dx, al
	mov dx, CRT_DATA_REG
	mov al, bh
	out dx, al

	mov dx, CRT_ADDR_REG
	mov al, CRT_CURSOR_L
	out dx, al
	mov dx, CRT_DATA_REG
	mov al, bl
	out dx, al

	popad
	ret

global put_str
put_str:
	push ebx
	push ecx

	xor ecx, ecx
	mov ebx, [esp + 3 * 4] ;拿到str addr,2个寄存器+retaddr

.goon:
	mov cl, [ebx]
	cmp cl, 0
	je .str_over
	push ecx
	call put_char
	add esp, 4
	inc ebx
	jmp .goon

.str_over:
	pop ecx
	pop ebx
	ret

;16进制显示整数
global put_int
put_int:
	pushad

	mov eax, [esp + 9 * 4] ;拿到整数

;解析32位数据填充put_int_buf
	mov edx, eax
	mov ebx, put_int_buf
	mov edi, 7
	mov ecx, 8
.16base_4bits:
	and edx, 0xF
	cmp edx, 9
	jg .is_A2F
	add edx, '0'
	jmp .store

.is_A2F:
	sub edx, 10
	add edx, 'A'

.store:
	mov [ebx + edi], dl
	dec edi
	shr eax, 4
	mov edx, eax
	loop .16base_4bits

;打印
.ready_to_print:
	push '0'
	call put_char
	add esp, 4
	push 'x'
	call put_char
	add esp, 4

;高位0不打印
	mov edi, 0
.skip_prefix_0:
	cmp edi, 8
	je .full0

.go_on_skip:
	mov cl, [put_int_buf + edi]
	inc edi
	cmp cl, '0'
	je .skip_prefix_0
	dec edi
	jmp .put_each_num

.full0:
	mov cl, '0'

.put_each_num:
	push ecx
	call put_char
	add esp, 4
	inc edi
	mov cl, [put_int_buf + edi]
	cmp edi, 8
	jl .put_each_num

	popad
	ret

section .data
put_int_buf:
	dq 0

