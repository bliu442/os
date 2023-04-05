;端口操作

[bits 32]

section .test

;@brief 读端口 一字节
;@param port 端口号
;@retval 读到的数据
global in_byte
in_byte:
	push ebp
	mov ebp, esp

	xor eax, eax

	mov edx, [ebp + 0x8]
	in al, dx

	leave
	ret

;@brief 写端口 一字节
;@param port 端口号
;@param value 写入的数据
global out_byte
out_byte:
	push ebp
	mov ebp, esp

	xor eax,eax

	mov edx, [ebp + 0x8]
	mov eax, [ebp + 0xC]
	out dx, al

	leave
	ret

;@brief 读端口 一字
;@param port 端口号
;@retval 读到的数据
global in_word
in_word:
	push ebp
	mov ebp, esp

	xor eax, eax
	
	mov edx, [ebp + 0x8]
	in ax, dx

	leave
	ret

;@brief 写端口 一字
;@param port 端口号
;@param value 写入的数据
global out_word
out_word:
	push ebp
	mov ebp, esp

	xor eax, eax

	mov ebx, [ebp + 0x8]
	mov eax, [ebp + 0xC]
	out dx, ax

	leave
	ret