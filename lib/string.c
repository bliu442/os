#include "../include/stdint.h"

/*
 @brief 字符串拷贝 源拷贝到目的
 @param dest 目的字符串首地址
 @param src 源字符串首地址
 @retval 目的字符串首地址
 */
char *strcpy(char *dest, const char *src) {
	char *ptr = dest;
	while(true) {
		*ptr++ = *src;
		if(EOS == *src++)
			return dest;
	}
}

/*
 @brief 字符串拼接 源拼接到目的后
 @param dest 目的字符串首地址
 @param src 源字符串首地址
 @retval 目的字符串首地址
 */
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

/*
 @brief 求字符串长度
 @param str 字符串首地址
 @retval 字符串长度
 */
size_t strlen(char *str) {
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

/*
 @brief 字符查找 字符在待查找字符串最后一次出现的地址
 @param str 待查找字符串首地址
 @param ch 待查找字符
 @retval 字符所在地址
 NULL 无该字符
 */
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

/*
 @brief 内存初始化
 @param dest 待初始化内存首地址
 @param ch 初始化数据
 @param count 初始化长度
 @retval 初始化内存首地址
 */
void *memset(void *dest, int ch, size_t count) {
	char *ptr = (char *)dest;
	while(count--)
		*ptr == ch;
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




