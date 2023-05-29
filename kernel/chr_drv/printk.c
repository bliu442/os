/* 内核格式化打印函数printk */

#include "../../include/kernel/print.h"
#include "../../include/stdarg.h"
#include "../../include/asm/system.h"
#include "../../include/kernel/sync.h"

static char buf[1024];
static lock_t printk_buf_lock;

void printk_init(void) {
	lock_init(&printk_buf_lock);
}

/*
 @brief 格式化打印
 @param fmt 字符串首地址
 @param ... 变参
 @retval 打印字符的个数
 */
int printk(const char *fmt, ...) {
	lock_acquire(&printk_buf_lock);

	va_list args;
	int i;
	int color = WHITE;

	va_start(args, fmt);
	va_list ptr = args;

	int magic = va_arg(args, int);
	if((magic & 0xFFFFFF00) == 0x5B3B5D00) {
		color = magic;	
		ptr += 4;
	}

	i = vsprintf(buf, fmt, ptr);
	va_end(args);

	i = console_write(buf, i, color);

	lock_release(&printk_buf_lock);
	return i;
}

/*
printk("%d, %d, %d\r", 0x1, 0x1234, 0x5aa5);
|  0x5aa5|参数压栈 ebp + 20
|  0x1234|参数压栈 ebp + 16
|     0x1|参数压栈 ebp + 12
|     fmt|参数压栈 ebp + 8
| retaddr|返回地址 ebp + 4
|.....ebp| ebp
|    args|局部变量 ebp - 4
|       i|局部变量 ebp - 8

va_start(args, fmt);
args = (va_list)&fmt + 4; 找到第二个参数的地址
*/