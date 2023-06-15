.PHONY:compiling clean

PWD = /home/liuben/bliuProject/bliu_os#工程绝对路径
BUILD = Build#编译生成文件存放路径

DIRS = boot init kernel/chr_drv kernel/asm lib kernel mm kernel/blk_drv fs
OBJS_DIRS := $(addprefix ./$(BUILD)/, $(DIRS))
DIRS := $(addprefix $(PWD)/, $(DIRS))

OBJS := $(addsuffix /*.o, $(OBJS_DIRS))
OBJS := $(wildcard $(OBJS))

RMS = bx_enh_dbg.ini .vscode/settings.json $(SYS_HD) $(BUILD) hdc40M.img hdd40M.img

SYS_HD = hda20M.img
FS_HD = hdb80M.img
HD_IMG = $(SYS_HD) $(FS_HD) hdc40M.img hdd40M.img

compiling:
	@set -e; \
	for dir in $(DIRS); \
	do \
		cd $$dir && $(MAKE) all; \
	done

all:$(HD_IMG) $(BUILD)/system.bin
	dd if=./$(BUILD)/boot/mbr.bin of=$(SYS_HD) bs=512 seek=0 count=1 conv=notrunc
	dd if=./$(BUILD)/boot/setup.bin of=$(SYS_HD) bs=512 seek=1 count=4 conv=notrunc
	dd if=./$(BUILD)/system.bin of=$(SYS_HD) bs=512 seek=5 count=100 conv=notrunc

clean:
	rm -rf $(RMS)

bochs:all
	bochs -q -f bochsrc

qemug:all
	qemu-system-i386 -m 32M \
	-hda $(SYS_HD) \
	-hdb $(FS_HD) \
	-hdc hdc40M.img \
	-hdd hdd40M.img \
	-S -s

qemu:all
	qemu-system-i386 -m 32M -boot c \
	-hda $(SYS_HD) \
	-hdb $(FS_HD) \
	-hdc hdc40M.img \
	-hdd hdd40M.img \

$(SYS_HD):
	bximage -q -func=create -hd=20 -sectsize=512 -imgmode=flat $@

#hdb80M.img1        2048  10239   8192    4M 83 Linux
#hdb80M.img2       10240  26623  16384    8M 83 Linux
#hdb80M.img3       26624 163295 136672 66.7M  5 扩展
#hdb80M.img5       28672  36863   8192    4M 83 Linux
#hdb80M.img6       38912  55295  16384    8M 83 Linux
#hdb80M.img7       57344  61439   4096    2M 83 Linux
#hdb80M.img8       63488  67583   4096    2M 83 Linux
#hdb80M.img9       69632  73727   4096    2M 83 Linux
#hdb80M.img10      75776 163295  87520 42.7M 83 Linux

$(FS_HD):
	bximage -q -func=create -hd=80 -sectsize=512 -imgmode=flat $@

hdc40M.img hdd40M.img:
	bximage -q -func=create -hd=40 -sectsize=512 -imgmode=flat $@

$(BUILD)/system.bin:$(BUILD)/kernel.elf
	objcopy -O binary $^ $@
	nm $^ | sort > $(BUILD)/system.map

$(BUILD)/kernel.elf:$(OBJS)
	ld -m elf_i386 $^ -o $@ -Ttext 0x10000


