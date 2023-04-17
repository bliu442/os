/* 文字模式显卡驱动 以显存地址为基础 显存地址 / 2 = 光标位置 */

#include "../../include/stdint.h"
#include "../../include/string.h"
#include "../../include/asm/io.h"
#include "../../include/kernel/sync.h"
#include "../../include/kernel/print.h"

#define CRT_ADDR_REG 0x3D4
#define CRT_DATA_REG 0x3D5

#define CRT_START_ADDR_H 0xC //屏幕显示起始字符的位置,从该字符处向后显示一页
#define CRT_START_ADDR_L 0xD
#define CRT_CURSOR_H 0xE //光标的位置
#define CRT_CURSOR_L 0xF

#define VIDEO_MEM_START 0xC00B8000
#define VIDEO_MEM_SIZE 0x8000
#define VIDEO_MEM_END VIDEO_MEM_START + VIDEO_MEM_SIZE

#define WIDTH 80
#define HEIGHT 25
#define ROW_SIZE (WIDTH * 2) //一行字节数
#define SCR_SIZE (ROW_SIZE * HEIGHT) //一页字节数

static uint32_t screen; //当前屏幕显示起始字符位置的显存地址
static uint32_t pos; //当前光标位置的显存地址
static uint32_t x, y; //当前光标坐标

#define ASCII_BS 0x08 // \b
#define ASCII_LF 0x0A // \n
#define ASCII_CR 0x0D // \r

static lock_t console_lock;

/* @brief 设置屏幕显示起始字符位置 */
static void set_screen(void) {
	out_byte(CRT_ADDR_REG, CRT_START_ADDR_H);
	out_byte(CRT_DATA_REG, ((screen - VIDEO_MEM_START) >> 9) & 0xff);
	out_byte(CRT_ADDR_REG, CRT_START_ADDR_L);
	out_byte(CRT_DATA_REG, ((screen - VIDEO_MEM_START) >> 1) & 0xff); // >>1 一个字符由两个字节构成,将内存差转换为位置
}

/* @brief 设置光标的位置 */
static void set_cursor(void) {
	out_byte(CRT_ADDR_REG, CRT_CURSOR_H);
	out_byte(CRT_DATA_REG, ((pos - VIDEO_MEM_START) >> 9) & 0xff);
	out_byte(CRT_ADDR_REG, CRT_CURSOR_L);
	out_byte(CRT_DATA_REG, ((pos - VIDEO_MEM_START) >> 1) & 0xff);
}

/* @brief 屏幕初始化 设置从VIDEO_MEM_START开始显示 所有显存置为0x0720 闪烁黑底白字空格 */
void console_init(void) {
	screen = VIDEO_MEM_START;
	pos = VIDEO_MEM_START;
	x = y = 0;
	set_cursor();
	set_screen();

	lock_init(&console_lock);

	uint16_t *ptr = (uint16_t *)VIDEO_MEM_START;
	while((uint32_t)ptr < VIDEO_MEM_END)
		*ptr++ = 0x0720;
}

static void console_acquire(void) {
	lock_acquire(&console_lock);
}

static void console_release(void) {
	lock_release(&console_lock);
}

/* @brief 向上滚动一行 */
static void scroll_up(void) {
	if(screen + SCR_SIZE + ROW_SIZE < VIDEO_MEM_END) {
		uint16_t *ptr = (uint16_t *)(screen + SCR_SIZE);
		for(size_t i = 0;i < WIDTH;++i)
			*ptr++ = 0x0720;
		screen += ROW_SIZE;
		pos += ROW_SIZE;
	}
	else { //超过显存最大地址,回绕一下
		memcpy((void *)VIDEO_MEM_START, (void *)screen, SCR_SIZE);
		pos -= (screen - VIDEO_MEM_START);
		screen = VIDEO_MEM_START;
	}
	set_screen();
}

/* @brief 换行 */
static void command_lf(void) {
	if(y + 1 < HEIGHT) {
		y++;
		pos += ROW_SIZE;
		return;
	}
	scroll_up();
}

/* @brief 回车 */
static void command_cr(void) {
	pos -= (x << 1); //从当前位置回到当前行第一个字符处
	x = 0;
}

/* @brief 删除一个字符 */
static void command_bs(void) {
	if(x) {
		--x;
		pos -= 2;
		*(uint16_t *)pos = 0x0720;
	}
}

/*
 @brief 向屏幕输出 特殊字符只实现换行 回车 删除
 @param buf 要输出的字符串首地址
 @param count 输出长度
 */
void console_write(char *buf, uint32_t count) {
	console_acquire();

	char ch;
	char *ptr = (char *)pos;
	
	while(count--) {
		ch = *buf++;
		switch(ch) {
			case ASCII_BS:
				command_bs();
				break;
			case ASCII_LF:
				command_lf();
				command_cr();
				break;
			case ASCII_CR:
				command_lf();
				command_cr();
				break;
			default:
				if(x >= WIDTH) {
					x -= WIDTH;
					command_lf();
					command_cr();
				}

				*ptr++ = ch;
				*ptr++ = 0x07;

				pos += 2;
				++x;
				break;
				
		}
	}
	set_cursor();

	console_release();
}

void console_put_char(char ch) {
	console_acquire();
	put_char(ch);
	console_release();
}

void console_put_str(char *str) {
	console_acquire();
	put_str(str);
	console_release();
}

void console_put_int(uint32_t num) {
	console_acquire();
	put_int(num);
	console_release();
}