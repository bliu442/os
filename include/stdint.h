#ifndef __STDINT_H__
#define __STDINT_H__

#define EOS '\0' //end of string

#define NULL ((void *)0)

#define bool _Bool
#define true 1
#define false 0

typedef unsigned int size_t;
typedef unsigned int ssize_t;

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef long long int64_t;
typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

#endif