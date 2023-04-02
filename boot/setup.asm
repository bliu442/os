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

	jmp $

