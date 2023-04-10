/*
 string functions 
 不知道为啥局部标签会报错
 */

#include "../include/string.h"

#if 0
char *strcpy(char *dest, const char *src) {
	char *ptr = dest;
	while(true) {
		*ptr++ = *src;
		if(EOS == *src++)
			return dest;
	}
}

char *strcat(char *dest, const char *src) {
	char *ptr = dest;
	while(*ptr != EOS)
		ptr++;
	while(true) {
		*ptr++ = *src;
		if(EOS == *src)
			return dest;
	}
}

size_t strlen(cosnt char *str) {
	char *ptr = str;
	while(*ptr != EOS) {
		ptr++;
	}
	return ptr - str;
}

/*
 @brief 字符串比较
 @param lhs 第一个字符串
 @param rhs 第二个字符串
 @retval 第一个字符串小 返回-1
 第一个字符串大 返回ASCII差值
 */
int strcmp(const char *lhs, const char *rhs) {
	if(*lhs == *rhs && *lhs != EOS && *rhs != EOS) {
		lhs++;
		rhs++;
	}
	return *lhs < *rhs ? -1 : *lhs > *rhs;
}

/*
 @brief 字符查找
 @param str 待查找字符串首地址
 @param ch 待查找字符
 @retval 字符所在地址
 NULL 无该字符
 */
char *strchr(const char *str, int ch) {
	char *ptr = (char *)str;
	while(true) {
		if(ch == *ptr)
			return ptr;
		else if(EOS == *ptr)
			return NULL;
	}
}

char *strrchr(const char *str, int ch) {
	char *last = NULL;
	char *ptr = (char *)str;
	while(true) {
		if(ch == *ptr)
			last = ptr;
		if(EOS == *ptr++)
			return last;
	}
}

/*
 @brief 内存比较
 @param lhs 第一段内存首地址
 @param rhs 第二段内存首地址
 @param count 比较长度
 @retval 第一段内存小 返回-1
 第一段内存大 返回ASCII差值
 相等返回0
 */
int memcmp(const void *lhs, const void *rhs, size_t count) {
	char *lptr = (char *)lhs;
	char *rptr = (char *)rhs;
	if(*lptr == *rptr && *lptr != EOS && *rptr != EOS) {
		lptr++;
		rptr++;
	}
	return *lptr < *rptr ? -1 : *lptr - *rptr;
}

void *memset(void *dest, int ch, size_t count) {
	char *ptr = (char *)dest;
	while(count--)
		*ptr == ch;
	return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
	char *ptr = (char *)dest;
	while(count--)
		*ptr++ = *(char *)(src++);
	return dest;
}

/*
 @brief 内存查找
 @param str 待查找内存首地址
 @param ch 待查找字符
 @param count 查找长度
 @retval 字符所在内存地址
 NULL 没找到*/
void *memchr(const void *str, int ch, size_t count) {
	char *ptr = (char *)str;
	while(count--) {
		if(ch == *ptr)
			return (void *)ptr;
		ptr++;
	}
	return NULL;
}
#else
/*
 @brief 字符串拷贝 源拷贝到目的
 @param dest 目的字符串首地址
 @param src 源字符串首地址
 @retval 目的字符串首地址
 */
char *strcpy(char *dest, const char *src) {
	__asm__("cld;"
		".strcpy_loop: lodsb;"
		"stosb;"
		"test al, al;"
		"jne .strcpy_loop"
		:: "S" (src), "D" (dest)
		);
	return dest;
}

/*
 @brief 字符串拼接 源拼接到目的后
 @param dest 目的字符串首地址
 @param src 源字符串首地址
 @retval 目的字符串首地址
 */
char *strcat(char *dest, const char *src) {
	__asm__("cld;"
		"repne scasb;"
		"dec %1;"
		".strcat_loop: lodsb;"
		"stosb;"
		"testb al, al;"
		"jne .strcat_loop"
		:: "S" (src), "D" (dest), "a" (0), "c" (0xFFFFFFFF)
		);
}

/*
 @brief 求字符串长度
 @param str 字符串首地址
 @retval 字符串长度
 */
size_t strlen(const char *s) {
	register size_t __res;
	__asm__("cld;"
		"repne scasb;"
		"not %0;"
		"dec %0"
		: "=c" (__res) : "D" (s), "a" (0), "0" (0xFFFFFFFF)
		);
	return __res;
}

/*
 @brief 字符串比较
 @param lhs 第一个字符串
 @param rhs 第二个字符串
 @retval 第一个字符串小 返回-1
 第一个字符串大 返回1
 */
int strcmp(const char *lhs, const char *rhs) {
	register int __res;
	__asm__("cld;"
		".strcmp_loop: lodsb;"
		"scasb;"
		"jne .strcmp_end;"
		"test al, al;"
		"jne .strcmp_loop;"
		"xor eax, eax;"
		"jmp .strcmp_return;"
		".strcmp_end: mov eax, 1;"
		"jl .strcmp_return;"
		"neg eax;"
		".strcmp_return:"
		: "=a" (__res) : "D" (lhs), "S" (rhs)
		);
	return __res;
}

/*
 @brief 字符查找
 @param str 待查找字符串首地址
 @param ch 待查找字符
 @retval 字符所在地址
 NULL 无该字符
 */
char *strchr(const char *str, int ch) {
	register char *__res;
	__asm__("cld;"
		"mov ah, al;"
		".strchr_loop: lodsb;"
		"cmp ah, al;"
		"je .strchr_end;"
		"test al, al;"
		"jne .strchr_loop;"
		"mov %1, 1;"
		".strchr_end: mov %0, %1;"
		"dec %0"
		: "=a" (__res) : "S" (str), "0" (ch)
		);
	return __res;
}

/*
 @brief 字符查找 字符在待查找字符串最后一次出现的地址
 @param str 待查找字符串首地址
 @param ch 待查找字符
 @retval 字符所在地址
 NULL 无该字符
 */
char *strrchr(const char *str, int ch) {
	register char *__res;
	__asm__("cld;"
		"mov ah, al;"
		".strrchr_loop: lodsb;"
		"cmp ah, al;"
		"jne .strrchr_end;"
		"mov %0, esi;"
		"dec %0;"
		".strrchr_end: test al, al;"
		"jne .strrchr_loop"
		: "=d" (__res) : "0" (0), "S" (str), "a" (ch)
		);
	return __res;
}

/*
 @brief 内存比较
 @param lhs 第一段内存首地址
 @param rhs 第二段内存首地址
 @param count 比较长度
 @retval 第一段内存小 返回-1
 第一段内存大 返回1
 相等返回0
 */
int memcmp(const void *lhs, const void *rhs, size_t count) {
	register int __res;
	__asm__("cld;"
		"repe cmpsb;"
		"je .memcmp_end;"
		"mov eax, 1;"
		"jl .memcmp_end;"
		"neg eax;"
		".memcmp_end:"
		: "=a" (__res) : "0" (0), "D" (lhs), "S" (rhs), "c" (count)
		);
	return __res;
}

/*
 @brief 内存初始化
 @param dest 待初始化内存首地址
 @param ch 初始化数据
 @param count 初始化长度
 @retval 初始化内存首地址
 */
void *memset(void *dest, int ch, size_t count) {
	__asm__("cld;"
		"rep stosb;"
		:: "a" (ch), "D" (dest), "c" (count)
		);
	return dest;
}

/*
 @brief 内存拷贝
 @param dest 目的内存空间首地址
 @param src 源内存空间首地址
 @param count 拷贝长度
 @retval 目的内存空间首地址
 */
void *memcpy(void *dest, const void *src, size_t count) {
	__asm__("cld;"
		"rep movsb;"
		:: "c" (count), "S" (src), "D" (dest)
	);
	return dest;
}

/*
 @brief 内存查找
 @param str 待查找内存首地址
 @param ch 待查找字符
 @param count 查找长度
 @retval 字符所在内存地址
 NULL 没找到*/
void *memchr(const void *str, int ch, size_t count) {
	register void *__res;
	if(!count)
		return NULL;
	__asm__("cld;"
		"repne scasb;"
		"je .memchr_end;"
		"mov %0, 1;"
		".memchr_end: dec %0;"
		: "=D" (__res) : "a" (ch), "D" (str), "c" (count)
	);
	return __res;
}
#endif




