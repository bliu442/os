;cpu根据中断向量号定位中断门描述符
;cpu进行特权级检查
;执行中断处理程序

[bits 32]

%define ERROR_CODE nop
%define IDT_INDEX push 0

extern exception_handler ;see interrupt.h
extern printk ;see print.h
extern clock_handler ;see interrupt.h
extern syscall_table ;see syscall.c
extern hd_handler ;see hd.c
extern keymap_handler ;see keyboard.c

section .data
global interrupt_handler_table
interrupt_handler_table: ;创建数组,.data段的合并

;|..........| 发生中断
;|....ss_old|
;|...esp_old| 跨态时会自动被压入,运行应用态代码时发生中断,保存r3堆栈
;|....eflags|
;|........cs|
;|.......eip| iret 从eip开始弹出数据到相应的寄存器,必须保证栈顶数据为eip
;|error_code| 有的中断有错误码,cpu会自动压入堆栈,没有错误码的手动压入一次数据,写宏好平栈
;------------ cpu自动压栈的数据
;|.idt_index| 中断向量号
;|.......eax|
;|.......ecx|
;|.......edx|
;|.......ebx|
;|.......esp|
;|.......ebp|
;|.......esi|
;|.......edi|
;------------ pushad/popad
;|........ds|
;|........es|
;|........fs|
;|........gs| <-esp
;1:中断向量号 2:平栈 3:中断处理函数
%macro INTERRUPT_HANDLER 3
section .text
global interrupt_handler_%1
interrupt_handler_%1:
	%2 ;为了平栈
	push %1 ;传递中断向量号进中断处理函数

	pushad

	push ds
	push es
	push fs
	push gs

	call %3

	pop gs
	pop fs
	pop es
	pop ds
	popad

	add esp, 8 ;平栈,让栈顶指向eip

	iret

section .data
	dd interrupt_handler_%1 ;

%endmacro

INTERRUPT_HANDLER 0x0, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x1, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x2, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x3, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x4, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x5, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x6, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x7, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x8, ERROR_CODE, exception_handler
INTERRUPT_HANDLER 0x9, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0xA, ERROR_CODE, exception_handler
INTERRUPT_HANDLER 0xB, ERROR_CODE, exception_handler
INTERRUPT_HANDLER 0xC, ERROR_CODE, exception_handler
INTERRUPT_HANDLER 0xD, ERROR_CODE, exception_handler
INTERRUPT_HANDLER 0xE, ERROR_CODE, exception_handler
INTERRUPT_HANDLER 0xF, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x10, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x11, ERROR_CODE, exception_handler
INTERRUPT_HANDLER 0x12, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x13, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x14, IDT_INDEX, exception_handler
INTERRUPT_HANDLER 0x15, IDT_INDEX, exception_handler

INTERRUPT_HANDLER 0x16, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x17, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x18, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x19, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x1A, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x1B, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x1C, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x1D, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x1E, IDT_INDEX, interrupt_handler_default ;reserved
INTERRUPT_HANDLER 0x1F, IDT_INDEX, interrupt_handler_default ;reserved

INTERRUPT_HANDLER 0x20, IDT_INDEX, clock_handler
INTERRUPT_HANDLER 0x21, IDT_INDEX, keymap_handler
INTERRUPT_HANDLER 0x22, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x23, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x24, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x25, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x26, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x27, IDT_INDEX, interrupt_handler_default

INTERRUPT_HANDLER 0x28, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x29, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x2A, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x2B, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x2C, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x2D, IDT_INDEX, interrupt_handler_default
INTERRUPT_HANDLER 0x2E, IDT_INDEX, hd_handler
INTERRUPT_HANDLER 0x2F, IDT_INDEX, hd_handler

global syscall_entry ;用户态进入内核态,cpu自动从tss里拿到r0堆栈栈顶 进入用户态时,r0堆栈所有数据变为无效数据 陷入内核时,从栈顶开始使用堆栈
syscall_entry:
	push 0 ;压入错误号
	push 0x80 ;压入中断向量号 跟INTERRUPT_HANDLER保持一致

	pushad

	push ds
	push es
	push fs
	push gs

	;调用syscall_table函数 堆栈
	;|.......edx|
	;|.......ecx|
	;|.......ebx|
	;|...retaddr|
	;进入sys_write函数
	push edx
	push ecx
	push ebx
	call [syscall_table + eax * 4]
	add esp, 0xC
	mov [esp + 11 * 4], eax ;将函数返回值放入栈中eax寄存器中,返回用户态时充当函数返回值
	
	pop gs
	pop fs
	pop es
	pop ds

	popad

	add esp, 8 ;平栈,让栈顶指向eip

	iret

section .text
global interrupt_handler_default
interrupt_handler_default:
	push msg
	call printk
	add esp, 4

	iret

global interrupt_exit
interrupt_exit:
	pop gs
	pop fs
	pop es
	pop ds
	popad

	cli
	add esp, 8 ;平栈,让栈顶指向eip

	iret
	
msg:
	db "interrupt_handler_default", 10, 13, 0


