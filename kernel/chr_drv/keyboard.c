/*
 8042键盘控制器 一次击键产生两次中断(通码 + 断码) 键盘扫描码 = 通码makecode + 断码breakcode; 断码 = 通码 | 0x80; 通码:最高位为0 按键处于按下状态
 键的状态改变,按键对应的扫描码发送到KBD_BUFF_PORT,之后8042向8259A发送中断信号
 KBD_BUFF_PORT 8b,中断一次只能读一字节
 拓展码 在扫描码前加0xE0 eg:右ALT = 0xE0 + KEY_ALT_L eg:按一次右ALT将产生四次中断 0xE0 0x38 0xE0 0xB8 将产生四次中断
 键盘中断处理程序必须读取KBD_BUFF_PORT,否则无法继续响应
 */

#include "../../include/stdint.h"
#include "../../include/asm/io.h"
#include "../../include/asm/system.h"
#include "../../include/kernel/ioqueue.h"

#define TAG "keyboard"
#define DEBUG_LEVEL 2
#include "../../include/kernel/debug.h"

#define KBD_BUFF_PORT 0x60

#define INV 0 //不可见字符
#define ESC 0x1B
#define DEL 0x7F

ioqueue_t keyboard_buf;

static char keymap[][4] = {
	/* 未与shift组合 与shift组合 false : 按键抬起 true : 按键按下 */
	
	/* 主键盘 */
	/* 0x00 KEY_NONE */				{INV, INV},
	/* 0x01 KEY_ESC */				{ESC, ESC},
	/* 0x02 KEY_1 */				{'1', '!'},
	/* 0x03 KEY_2 */				{'2', '@'},
	/* 0x04 KEY_3 */				{'3', '#'},
	/* 0x05 KEY_4 */				{'4', '$'},
	/* 0x06 KEY_5 */				{'5', '%'},
	/* 0x07 KEY_6 */				{'6', '^'},
	/* 0x08 KEY_7 */				{'7', '&'},
	/* 0x09 KEY_8 */				{'8', '*'},
	/* 0x0A KEY_9 */				{'9', '('},
	/* 0x0B KEY_0 */				{'0', ')'},
	/* 0x0C KEY_MINUS */			{'-', '_'},
	/* 0x0D KEY_EQUAL */			{'=', '+'},
	/* 0x0E KEY_BACKSPACE */		{'\b', '\b'},
	/* 0x0F KEY_TAB */				{'\t', '\t'},
	/* 0x10 KEY_Q */				{'q', 'Q'},
	/* 0x11 KEY_W */				{'w', 'W'},
	/* 0x12 KEY_E */				{'e', 'E'},
	/* 0x13 KEY_R */				{'r', 'R'},
	/* 0x14 KEY_T */				{'t', 'T'},
	/* 0x15 KEY_Y */				{'y', 'Y'},
	/* 0x16 KEY_U */				{'u', 'U'},
	/* 0x17 KEY_I */				{'i', 'I'},
	/* 0x18 KEY_O */				{'o', 'O'},
	/* 0x19 KEY_P */				{'p', 'P'},
	/* 0x1A KEY_BRACKET_L */		{'[', '{'},
	/* 0x1B KEY_BRACKET_R */		{']', '}'},
	/* 0x1C KEY_ENTER */			{'\n', '\n'},
	/* 0x1D KEY_CTRL_L */			{INV, INV},
	/* 0x1E KEY_A */				{'a', 'A'},
	/* 0x1F KEY_S */				{'s', 'S'},
	/* 0x20 KEY_D */				{'d', 'D'},
	/* 0x21 KEY_F */				{'f', 'F'},
	/* 0x22 KEY_G */				{'g', 'G'},
	/* 0x23 KEY_H */				{'h', 'H'},
	/* 0x24 KEY_J */				{'j', 'J'},
	/* 0x25 KEY_K */				{'k', 'K'},
	/* 0x26 KEY_L */				{'l', 'L'},
	/* 0x27 KEY_SEMICOLON */		{';', ':'},
	/* 0x28 KEY_QUOTE */			{'\'', '"'},
	/* 0x29 KEY_BACKQUOTE */		{'`', '~'},
	/* 0x2A KEY_SHIFT_L */			{INV, INV},
	/* 0x2B KEY_BACKSLASH */		{'\\', '|'},
	/* 0x2C KEY_Z */				{'z', 'Z'},
	/* 0x2D KEY_X */				{'x', 'X'},
	/* 0x2E KEY_C */				{'c', 'C'},
	/* 0x2F KEY_V */				{'v', 'V'},
	/* 0x30 KEY_B */				{'b', 'B'},
	/* 0x31 KEY_N */				{'n', 'N'},
	/* 0x32 KEY_M */				{'m', 'M'},
	/* 0x33 KEY_COMMA */			{',', '<'},
	/* 0x34 KEY_POINT */			{'.', '>'},
	/* 0x35 KEY_SLASH */			{'/', '?'},
	/* 0x36 KEY_SHIFT_R */			{INV, INV},
	/* 0x37 KEY_STAR */				{'*', '*'},
	/* 0x38 KEY_ALT_L */			{INV, INV},
	/* 0x39 KEY_SPACE */			{' ', ' '},
	/* 0x3A KEY_CAPSLOCK */			{INV, INV},

	/* 0x3B KEY_F1 */				{INV, INV},
	/* 0x3C KEY_F2 */				{INV, INV},
	/* 0x3D KEY_F3 */				{INV, INV},
	/* 0x3E KEY_F4 */				{INV, INV},
	/* 0x3F KEY_F5 */				{INV, INV},
	/* 0x40 KEY_F6 */				{INV, INV},
	/* 0x41 KEY_F7 */				{INV, INV},
	/* 0x42 KEY_F8 */				{INV, INV},
	/* 0x43 KEY_F9 */				{INV, INV},
	/* 0x44 KEY_F10 */				{INV, INV},
	
	/* 0x45 KEY_NUMLOCK */			{INV, INV},
	/* 0x46 KEY_SCRLOCK */			{INV, INV},
	/* 0x47 KEY_PAD_7 Home */		{'7', INV},
	/* 0x48 KEY_PAD_8 Up */			{'8', INV},
	/* 0x49 KEY_PAD_9 PageUp */		{'9', INV},
	/* 0x4A KEY_PAD_MINUS */		{'-', '-'},
	/* 0x4B KEY_PAD_4 Left */		{'4', INV},
	/* 0x4C KEY_PAD_5 */			{'5', INV},
	/* 0x4D KEY_PAD_6 Right */		{'6', INV},
	/* 0x4E KEY_PAD_PLUS */			{'+', '+'},
	/* 0x4F KEY_PAD_1 End */		{'1', INV},
	/* 0x50 KEY_PAD_2 Down */		{'2', INV},
	/* 0x51 KEY_PAD_3 Pgdn */		{'3', INV},
	/* 0x52 KEY_PAD_0 Ins */ 		{'0', INV},
	/* 0x53 KEY_PAD_POINT Delete */	{'.', DEL},
	
	/* 0x54 KEY_NONE */				{INV, INV},
	/* 0x55 KEY_NONE */				{INV, INV},
	/* 0x56 KEY_NONE */				{INV, INV},
	/* 0x57 KEY_F11 */				{INV, INV},
	/* 0x58 KEY_F12 */				{INV, INV},
	/* 0x59 KEY_NONE */				{INV, INV},
	/* 0x5A KEY_NONE */				{INV, INV},
	/* 0x5B KEY_WIN_L */			{INV, INV},
	/* 0x5C KEY_WIN_R */			{INV, INV},
};

#define KEY_NUMLOCK			0x45
#define KEY_CAPSLOCK		0x3A
#define KEY_SCRLOCK			0x46

#define KEY_CTRL_L			0x1D
#define KEY_ALT_L			0x38
#define KEY_SHIFT_L			0x2A
#define KEY_SHIFT_R			0x36

static bool extcode_state; //拓展码状态

static bool capslock_state; //大写锁定
static bool scrlock_state; //滚动锁定
static bool numlock_state; //数字锁定

#define ctrl_state (keymap[KEY_CTRL_L][2] || keymap[KEY_CTRL_L][3]) //ctrl键状态
#define alt_state (keymap[KEY_ALT_L][2] || keymap[KEY_ALT_L][3]) //alt键状态
#define shift_state (keymap[KEY_SHIFT_L][2] || keymap[KEY_SHIFT_R][2]) //shift键状态

void keymap_handler(void) {
	uint8_t ext = 2; //默认非拓展码

	uint16_t scancode = in_byte(KBD_BUFF_PORT); //1.读取扫描码
	INFO("%#x\r", scancode);

	if(scancode == 0xE0) { //2.判断是否为拓展码
		extcode_state = true; //如果是拓展码继续接收扫描码
		return;
	}

	if(extcode_state) { //扫描码合并
		ext = 3;
		scancode |= 0xE000;
		extcode_state = false;
	}

	uint16_t makecode = (scancode & 0x7F); //3.拿到通码

	bool breakcode = (scancode & 0x0080); //4.判断按键是否抬起
	if(breakcode) {
		keymap[makecode][ext] = false; //恢复按键状态
		return; //按键抬起
	}

	keymap[makecode][ext] = true; //5.按键未抬起 记录按键状态 组合按键使用

	switch(makecode) {
		case KEY_NUMLOCK :
			numlock_state = !numlock_state;
			break;
		case KEY_CAPSLOCK :
			capslock_state = !capslock_state;
			break;
		case KEY_SCRLOCK :
			scrlock_state = !scrlock_state;
			break;
	}

	bool shift = false;
	if(capslock_state)
		shift = !shift;
	if(shift_state)
		shift = !shift;

	char ch = keymap[makecode][shift];
	if(ch == INV)
		return;

	if(!ioqueue_full(&keyboard_buf))
		ioqueue_putchar(&keyboard_buf, ch);
}
