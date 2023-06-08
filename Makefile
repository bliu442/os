.PHONY:compiling clean

PWD = /home/liuben/bliuProject/bliu_os#工程绝对路径
BUILD = Build#编译生成文件存放路径

DIRS = boot init kernel/chr_drv kernel/asm lib kernel mm kernel/blk_drv
OBJS_DIRS := $(addprefix ./$(BUILD)/, $(DIRS))
DIRS := $(addprefix $(PWD)/, $(DIRS))

OBJS := $(addsuffix /*.o, $(OBJS_DIRS))
OBJS := $(wildcard $(OBJS))

RMS = bx_enh_dbg.ini .vscode/settings.json $(HD_IMG) $(BUILD) hdb.img hdc.img hdd.img

HD_IMG = hd20M.img

compiling:
	@set -e; \
	for dir in $(DIRS); \
	do \
		cd $$dir && $(MAKE) all; \
	done

all:$(HD_IMG) $(BUILD)/system.bin hdb.img hdc.img hdd.img
	dd if=./$(BUILD)/boot/mbr.bin of=$(HD_IMG) bs=512 seek=0 count=1 conv=notrunc
	dd if=./$(BUILD)/boot/mbr.bin of=hdb.img bs=512 seek=0 count=1 conv=notrunc
	dd if=./$(BUILD)/boot/mbr.bin of=hdc.img bs=512 seek=0 count=1 conv=notrunc
	dd if=./$(BUILD)/boot/mbr.bin of=hdd.img bs=512 seek=0 count=1 conv=notrunc
	dd if=./$(BUILD)/boot/setup.bin of=$(HD_IMG) bs=512 seek=1 count=4 conv=notrunc
	dd if=./$(BUILD)/system.bin of=$(HD_IMG) bs=512 seek=5 count=100 conv=notrunc

clean:
	rm -rf $(RMS)

bochs:all
	bochs -q -f bochsrc

qemug:all
	qemu-system-i386 -m 32M \
	-hda $(HD_IMG) \
	-hdb ./hdb.img \
	-hdc ./hdc.img \
	-hdd ./hdd.img \
	-S -s

qemu:all
	qemu-system-i386 -m 32M -boot c -hda $(HD_IMG) \
	-hdb ./hdb.img \
	-hdc ./hdc.img \
	-hdd ./hdd.img \

$(HD_IMG) hdb.img hdc.img hdd.img:
	bximage -q -func=create -hd=20 -sectsize=512 -imgmode=flat $@

$(BUILD)/system.bin:$(BUILD)/kernel.elf
	objcopy -O binary $^ $@
	nm $^ | sort > $(BUILD)/system.map

$(BUILD)/kernel.elf:$(OBJS)
	ld -m elf_i386 $^ -o $@ -Ttext 0x10000


