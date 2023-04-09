/*
 8259a中断控制器初始化 必须依次写入ICW1-ICW4 再写入OCW1
 */

#include "../include/kernel/interrupt.h"
#include "../include/asm/io.h"
#include "../include/kernel/print.h"

char *messages[] = {
	"#DE Divide Error",
	"#DB Debug Exception",
	"--- NMI Interrupt",
	"#BP Breakpoint Exception",
	"#OF Overflow Exception",
	"#BR BOUND Range Exceeded Exception",
	"#UD Invalid Opcode Exception",
	"#NM Device Not Available Exception",
	"#DF Double Fault Exception",
	"--- Coprocessor Segment Overrun",
	"#TS Invalid TSS Exception",
	"#NP Segment Not Present",
	"#SS Stack Fault Exception",
	"#GP General Protection Exception",
	"#PF Page-Fault Exception",
	"--- (Inter reserved)",
	"#MF x87 FPU Floating-Point Error",
	"#AC Alignment Check Exception",
	"#MC Machine-Check Exception",
	"#XF SIMD Floating-Point Exception",
	"#VE Virtualization Exception",
	"#CP Control Protection Exception",
};

void pic_init(void) {
	out_byte(PIC_M_CTRL, 0x11); //ICW1
	out_byte(PIC_M_DATA, 0x20); //ICW2 IRQ[0-7]映射到0x20-0x27
	out_byte(PIC_M_DATA, 0x04); //ICW3 IRQ2接从片
	out_byte(PIC_M_DATA, 0x07); //ICW4 自动EOI

	out_byte(PIC_S_CTRL, 0x11); //ICW1
	out_byte(PIC_S_DATA, 0x28); //ICW2 IRQ[8-15]映射到0x28-0x2F
	out_byte(PIC_S_DATA, 0x02); //ICW2 从片连接到主片IRQ2上
	out_byte(PIC_S_DATA, 0x03); //ICW2

	out_byte(PIC_M_DATA, 0xFF); //OCW1 所有外部中断都屏蔽
	out_byte(PIC_S_DATA, 0xFF); //OCW1
}

/*
 |..........| 发生中断
 |....ss_old|
 |...esp_old| 跨态时会自动被压入,运行应用态代码时发生中断,保存堆栈,后面再搞
 |....eflags|
 |........cs|
 |.......eip| iret 从eip开始弹出数据到相应的寄存器,必须保证栈顶数据为eip
 |error_code| 有的中断有错误码,cpu会自动压入堆栈,没有错误码的动压入一次数据,写宏好平栈
 ------------ cpu自动压栈的数据
 |.idt_index| 中断向量号
 |.......eax|
 |.......ecx|
 |.......edx|
 |.......ebx|
 |.......esp|
 |.......ebp|
 |.......esi|
 |.......edi|
 ------------ pushad/popad
 |........ds|
 |........es|
 |........fs|
 |........gs| <-esp
 */
void exception_handler(uint32_t gs, uint32_t fs, uint32_t es, uint32_t ds, uint32_t edi, uint32_t esi,
	uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax,
	uint32_t idt_index, uint32_t error_code, uint32_t eip, uint32_t cs, uint32_t eflags) {
	put_char('\r');
	put_str("EXCEPTION : ");
	put_str(messages[idt_index]);
	put_char('\r');
	put_str("idt_index : ");
	put_int(idt_index);
	put_char('\r');
	put_str("eip       : ");
	put_int(eip);
	put_char('\r');
	put_str("cs        : ");
	put_int(cs);
	put_char('\r');
	put_str("eflags    : ");
	put_int(eflags);
	put_char('\r');

	while(true);
}