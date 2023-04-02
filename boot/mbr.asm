;mbr被bios加载到内存0x7C00处
;bios jmp 0:0x7C00 跳到mbr

[BITS 16]
[ORG 0x7C00]

SECTION MBR
	xchg bx, bx

	mov ax, cs
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov sp, 0x7C00

;int 0x10 bios中断设置显示器输出模式
;in al = 3 文本25 * 80 16色
	mov ah, 0x0
	mov al, 3
	int 0x10

	xchg bx, bx

;int 0x10 读取光标位置
;in bh = 0 页码
;out ch = 光标开始行 cl = 光标结束行
;out dh = 光标所在行 dl = 光标锁在列
	mov ah, 0x3
	mov bh, 0
	int 0x10

	xchg bx, bx

;int 0x10 bios中断打印字符串
;in al = 1 字符串只含显示字符，显示后光标位置改变 bh = 0 页码 bl = 2 属性
;in cx = 5 字符串长度 es:bp = message 字符串地址
;in (dh,dl) 坐标(行,列)
	mov bh, 0
	mov bl, 2

	mov ax, message
	mov bp, ax
	mov cx, 5

	mov dh, 1
	mov dl, 40

	mov ah, 0x13
	mov al, 0x1
	int 0x10

	xchg bx, bx

	mov ax, 0xB800
	mov gs, ax

	mov byte [gs:0x0], '1'
	mov byte [gs:0x1], 0xA4

	mov byte [gs:0x2], ' '
	mov byte [gs:0x3], 0xA4

	mov byte [gs:0x4], 'M'
	mov byte [gs:0x5], 0xA4

	mov byte [gs:0x6], 'B'
	mov byte [gs:0x7], 0xA4

	mov byte [gs:0x8], 'R'
	mov byte [gs:0x9], 0xA4

	jmp $

	message db "1 MBR"
	times 510 - ($ - $$) db 0
	db 0x55, 0xaa


