/*
 8253可编程定时器 clk = 1.19318MHz
 频率(hz) = 1(s) / 时间(s)
 1Hz <=> 1s
 1KHz <=> 1ms
 1MHz <=> 1us

 1193180 / 中断信号频率 = 计数器初始值
 */

#include "../../include/stdint.h"
#include "../../include/asm/io.h"
#include "../../include/kernel/print.h"
#include "../../include/asm/system.h"
#include "../../include/kernel/sched.h"
#include "../../include/kernel/thread.h"
#include "../../include/kernel/debug.h"

#define PIT_CHAN0_REG 0x40
#define PIT_CTRL_REG 0x43

#define COUNTER_NO_0 (0 << 6)
#define COUNTER0_RW (3 << 4)
#define COUNTER0_MODE (2 << 1)
#define COUNTER0_BCD (0)

#define TIME 10 //设置10ms一次时钟中断
#define HZ 100 //将时间转换为频率
#define OSCILLATOR 1193182 //定时器频率
#define CLOCK_COUNTER (OSCILLATOR / HZ) //寄存器的初始计数值

uint32_t systicks;

void clock_init(void) {
	out_byte(PIT_CTRL_REG, COUNTER_NO_0 | COUNTER0_RW | COUNTER0_MODE | COUNTER0_BCD); //0b00_11_010_0
	out_byte(PIT_CHAN0_REG, CLOCK_COUNTER & 0xFF);
	out_byte(PIT_CHAN0_REG, CLOCK_COUNTER >> 8 & 0xFF);
}

void clock_handler(void) {
	systicks++;

	task_t *current = running_thread();
	ASSERT(current->magic != 0x5aa555aa)

	current->elapsed_ticks++;

	if(current->ticks == 0)
		schedule();
	else
		current->ticks--;
}

static void sleep(uint32_t ticks) {
	uint32_t start_ticks = systicks;
	while(systicks != start_ticks + ticks)
		thread_yield();
}

/*
 @brief 毫秒级延时 以时钟中断为基础,~20ms误差
 @param time 单位ms
 */
void sleep_ms(uint32_t time) {
	uint32_t sleep_ticks = DIV_ROUND_UP(time, TIME);
	sleep(sleep_ticks);
}
