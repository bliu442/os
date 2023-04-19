#ifndef __STDARG_H__
#define __STDATG_H__

typedef char * va_list;

#define va_start(p, count) (p = (va_list)&count + sizeof(char *)) //拿到第二个参数的在堆栈中地址
/*
 从p处取出四字节数据 转换成t类型 将p向高地址移动四字节
 类似于*ptr++
 */
#define va_arg(p, t) (*(t*)((p += sizeof(char *)) - sizeof(char *)))
#define va_end(p) (p = 0) //防止空指针

#endif