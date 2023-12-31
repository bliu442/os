/*
 8259a中断控制器初始化 必须依次写入ICW1-ICW4 再写入OCW1
 */

#include "../include/kernel/interrupt.h"
#include "../include/asm/io.h"
#include "../include/kernel/print.h"
#include "../include/asm/system.h"
#include "../include/kernel/global.h"

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
    printk("\r=============================\r");
    printk("EXCEPTION : %s\r", messages[idt_index]);
    printk("   VECTOR : %#x\r", idt_index);
    printk("   EFLAGS : %#x\r", eflags);
    printk("       CS : %#x\r", cs);
    printk("      EIP : %#x\r", eip);
    printk("      ESP : %#x\r", esp);

	while(true);
}

/*
 @brief 获取eflags if位状态
 @retval if_sti/if_cli
 */
static if_enum_t interrupt_get_status(void) {
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (EFLAGS_IF & eflags) ? if_sti : if_cli;
}

/*
 @brief 打开中断并返回if位之前状态
 @retval if_sti/if_cli
 */
static if_enum_t interrupt_enable(void) {
	if_enum_t old_status;
	if(if_sti == interrupt_get_status())
		old_status = if_sti;
	else {
		old_status = if_cli;
		STI
	}

	return old_status;
}

/*
 @brief 关闭中断并返回if位之前状态
 @retval if_sti/if_cli
 */
if_enum_t interrupt_disable(void) {
	if_enum_t old_status;
	if(if_sti == interrupt_get_status()) {
		old_status = if_sti;
		CLI
	} else
		old_status = if_cli;
	
	return old_status;
}

/*
 @brief 根据if位旧状态 设置if位
 @retval if_sti/if_cli
 */
if_enum_t interrupt_set_status(if_enum_t status) {
	return status & if_sti ? interrupt_enable() : interrupt_disable();
}