.PHONY:clean

DIR = ../Build/boot

all:$(DIR) $(DIR)/mbr.bin #$(DIR)/setup.bin

$(DIR)/%.bin:%.asm
	nasm $^ -o $@

$(DIR):
	mkdir -p $@

clean:
	rm -rf $(DIR)

