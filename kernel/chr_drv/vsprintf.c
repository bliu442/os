/* 字符串格式化输出函数sprintf */

#include "../../include/stdarg.h"
#include "../../include/string.h"

#define is_digit(c) ((c) >= '0' && (c) <= '9')

/*
 @brief 将首字符为数字的字符串中的数字提取变成整形数 字符串向后移动
 @param s 字符串的二级指针
 @retval 转换出来的整形数
 */
static int skip_atoi(const char **s) {
	int i = 0;
	while(is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

#define ZEROPAD	1
#define SIGN	2 //符号控制
#define PLUS	4
#define SPACE	8
#define LEFT	16
#define SPECIAL	32 //十六进制 0x 八进制0 浮点数显示小数点
#define SMALL	64

#define do_div(n, base) ({ \
	int __res; \
	__asm__("div %4" \
		:"=a" (n), "=d" (__res) \
		:"0" (n), "1" (0), "r" (base)); \
	__res; \
})

static char *number(char *str, int num, int base, int size, int precision, int type) {
	char c, sign, tmp[36];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if(type & SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if(type & LEFT) type &= ~ZEROPAD;
	if(base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';
	if(type & SIGN && num < 0) {
		sign = '-';
		num = -num;
	} else
		sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	if(sign) --size;
	if(type & SPECIAL) {
		if(base == 16) size -= 2;
		else if(base == 8) --size;
	}

	i = 0;
	if(num == 0)
		tmp[i++] = '0';
	else while (num != 0)
		tmp[i++] = digits[do_div(num, base)];
	if(i > precision) precision = i;
	size -= precision;
	if(!(type & (ZEROPAD + LEFT)))
		while(size-->0)
			*str++ = ' ';
	if(sign)
		*str++ = sign;
	if(type & SPECIAL) {
		if(base == 8)
			*str++ = '0';
		else if(base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}
	if(!(type & LEFT))
		while(size-->0)
			*str++ = c;
	while(i < precision--)
		*str++ = '0';
	while(i-->0)
		*str++ = tmp[i];
	while(size-->0)
		*str++ = ' ';
	return str;
}

/*
 @brief 字符串格式化输出
 @param buf 输出字符缓冲区
 @param fmt 格式字符串
 @param args 堆栈传参
 @retval 转换好的字符串长度

 @输入字符串格式%[flags][width][.precision][length]
 @根据%个数确定变参个数
 */
int vsprintf(char *buf, const char *fmt, va_list args) {
	int len;
	int i;
	char *str; //存放转换的字符串
	char *s;
	int *ip;
	int flags; //输出格式控制
	int field_width; //宽度
	int precision; //精度
	int qualifier; //长度修饰

	/* %匹配 */
	for(str = buf;*fmt;++fmt) {
		if(*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		/* flags匹配 */
		flags = 0;
		repeat:
			++fmt;
			switch(*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
			}
		
		/* width匹配 */
		field_width = -1;
		if(is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if(*fmt == '*') {
			++fmt;
			/* field_width = *(int *)((args += 4) - 4); 按格式取出数据,并将args指针指向下个参数 */
			field_width = va_arg(args, int); //堆栈传入参数，读参数
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* precision匹配 */
		precision = -1;
		if(*fmt == '.') {
			++fmt;
			if(is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if(*fmt == '*') {
				++fmt;
				precision = va_arg(args, int);
			}
			if(precision < 0)
				precision = 0;
		}
		
		/* length匹配 */
		qualifier = -1;
		if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}
		
		switch(*fmt) {
			case 'c':
				if(!(flags & LEFT))
					while(--field_width > 0)
						*str++ = ' ';
				*str++ = (unsigned char)va_arg(args, int);
				while(--field_width > 0)
					*str++ = ' ';
				break;

			case 's':
				s = va_arg(args, char *);
				len = strlen(s);
				if(precision < 0)
					precision = len;
				else if(len > precision)
					len = precision;

				if(!(flags & LEFT))
					while(len < field_width--)
						*str++ = ' ';
				for(i = 0;i < len;++i)
					*str++ = *s++;
				while(len < field_width--)
					*str++ = ' ';
				break;
				
			case 'o':
				str = number(str, va_arg(args, unsigned long), 8, field_width, precision, flags);
				break;

			case 'p':
				if(field_width == -1) {
					field_width = 8;
					flags = ZEROPAD;
				}
				str = number(str, (unsigned long)va_arg(args, void *), 16, field_width, precision, flags);
				break;

			case 'x':
				flags |= SMALL;
			case 'X':
				str = number(str, va_arg(args, unsigned long), 16, field_width, precision, flags);
				break;

			case 'd':
			case 'i':
				flags |= SIGN;
			case 'u':
				str = number(str, va_arg(args, unsigned long), 10, field_width, precision, flags);
				break;

			case 'n':
				ip = va_arg(args, int *);
				*ip = (str - buf);
				break;

			default:
				if(*fmt != '%')
					*str++ = '%';
				if(*fmt)
					*str++ = *fmt;
				else
					--fmt;
				break;
		}
	}
	*str = '\0'; //转换结束
	return str - buf;
}
