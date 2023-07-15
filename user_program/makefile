.PHONY:all compiling ld clean

GCCFLAGS = -m32 #32位程序
GCCFLAGS += -masm=intel
GCCFLAGS += -fno-builtin #不需要gcc内置函数
GCCFLAGS += -nostdinc #不需要标准头函数
GCCFLAGS += -fno-pic #不需要位置无关的代码 position independent code
GCCFLAGS += -fno-pie #不需要位置无关的可执行程序 position independent executable
GCCFLAGS += -nostdlib #不需要标准库
GCCFLAGS += -fno-stack-protector #不需要栈保护
GCCFLAGS += -w -Wno-address-of-packed-member #地址不对齐
GCCFLAGS := $(strip $(GCCFLAGS))

DEBUG := -g

all:compiling ld
	xxd -i main.elf main.elf.h
	xxd -i main.o main.o.h

compiling:
	gcc $(GCCFLAGS) $(DEBUG) -c main.c -o main.o -I ../include/stdio.h

ld:
	ld -m elf_i386 main.o ../Build/lib/stdio.o ../Build/kernel/chr_drv/vsprintf.o ../Build/lib/write.o \
	../Build/lib/string.o ../Build/lib/error.o \
	-o main.elf -e main

clean:
	rm -rf main.o main.elf
