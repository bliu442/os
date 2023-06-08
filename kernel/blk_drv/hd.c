#include "../../include/kernel/hd.h"
#include "../../include/string.h"
#include "../../include/asm/system.h"
#include "../../include/asm/io.h"

#define TAG "disk"
#include "../../include/kernel/debug.h"

#define HD_DATA(channel)		(channel->port_base + 0) //0x1F0 16位寄存器
#define HD_ERROR(channel)		(channel->port_base + 1) //其余8位寄存器
#define HD_FEATURE(channel)		(HD_ERROR(channel))
#define HD_SECT_CNT(channel)	(channel->port_base + 2)
#define HD_LBA_L(channel)		(channel->port_base + 3)
#define HD_LBA_M(channel)		(channel->port_base + 4)
#define HD_LBA_H(channel)		(channel->port_base + 5)
#define HD_DEV(channel)			(channel->port_base + 6)
#define HD_STATUS(channel)		(channel->port_base + 7)
#define HD_CMD(channel)			(HD_STATUS(channel))
#define HD_CTL(channel)			(channel->port_base + 0x206) //0x3F6

/* HD_STATUS */
#define ERR_STAT	0x1
#define INDEX_STAT	0x2
#define ECC_STAT	0x4
#define DRQ_STAT	0x8 //数据准备好
#define SEEK_STAT	0x10 //读/写磁头就位
#define WRERR_STAT	0x20
#define READY_STAT	0x40 //就绪
#define BUSY_STAT	0x80 //忙

/* HD_CMD */
#define CMD_READ		0x20 //读扇区
#define CMD_WRITE		0x30 //写扇区
#define CMD_IDENTIFY	0xEC //获取硬盘信息

/* HD_DEV */
#define DEV_MBS		0xA0 //固定
#define DEV_LBA		0x40 //lba28寻址
#define DEV_DEV		0x10 //主/从盘

#define HD_NUMBER_ADDR 0x475 //bios检测内存写的(物理地址)
#define DIV_ROUND_UP(X, STEP) (((X) + (STEP) - 1) / (STEP))

const uint8_t channel_number = 2;
hd_channel_t channels[2];

/*
 boch
 1:ata0-master: type=disk, path="hd20M.img", mode=flat
 2:ata0-slave: type=disk, path="hdb.img", mode=flat
 3:ata1-master: type=disk, path="hdc.img", mode=flat
 4:ata1-slave: type=disk, path="hdd.img", mode=flat
 
 qemu
 1:-hda $(HD_IMG)
 2:-hdb ./hdb.img
 3:-hdc ./hdc.img
 4:-hdd ./hdd.img
 
 1,2盘对应ata0 3,4盘对应ata1
 */
void hd_init(void) {
	uint8_t hd_number = *(uint8_t *)(HD_NUMBER_ADDR);
	INFO("disk number : %d\r", hd_number);

	hd_channel_t *channel = NULL;
	disk_t *disk = NULL;
	uint8_t channel_no = 0, disk_no = 0;
	while(channel_no < channel_number) {
		channel = &channels[channel_no];

		switch(channel_no) {
			case 0:
				memcpy(channel->name, "ata0", sizeof("ata0"));
				channel->port_base = 0x1F0;
				channel->irq_no = 14 + 0x20;
				break;
			case 1:
				memcpy(channel->name, "ata1", sizeof("ata1"));
				channel->port_base = 0x170;
				channel->irq_no = 15 + 0x20;
				break;
		}

		lock_init(&channel->lock);
		semaphore_init(&channel->disk_done, 0); //用于等待硬盘异步读写完成

		channel->expecting_intrrupt = false;

		while(disk_no < 2) {
			disk = &channel->disk[disk_no];

			disk->channel = channel;
			disk->dev_no = disk_no;

			switch(disk_no) {
				case 0:
					strcat(disk->name, channel->name);
					strcat(disk->name, "-M");
					break;
				case 1:
					strcat(disk->name, channel->name);
					strcat(disk->name, "-S");
					break;
			}

			disk_no++;
		}

		disk_no = 0;
		channel_no++;
	}
}

static void out_param(disk_t *hd, uint32_t lba, uint8_t count) {
	out_byte(HD_SECT_CNT(hd->channel), count);

	out_byte(HD_LBA_L(hd->channel), lba);
	out_byte(HD_LBA_M(hd->channel), lba >> 8);
	out_byte(HD_LBA_H(hd->channel), lba >> 16);
	out_byte(HD_DEV(hd->channel), lba >> 24 | DEV_MBS | DEV_LBA | (hd->dev_no ? DEV_DEV : 0));
}

static void out_cmd(hd_channel_t *channel, uint8_t cmd) {
	channel->expecting_intrrupt = true;
	out_byte(HD_CMD(channel), cmd);
}

/*
 @brief 阻塞方式读硬盘
 @param hd 硬盘
 @param lba 要读的起始地址
 @param count 读多少扇区
 @param buf 读到哪
 */
void hd_read_sector(disk_t *hd, uint32_t lba, uint8_t count, void *buf) {
	uint8_t status = 0;
	while(1) {
		status = in_byte(HD_STATUS(hd->channel));
		if((status & BUSY_STAT) != BUSY_STAT)
			break;
	}

	out_param(hd, lba, count);
	out_cmd(hd->channel, CMD_READ);

	while(1) {
		status = in_byte(HD_STATUS(hd->channel));
		if((status & (READY_STAT | SEEK_STAT | DRQ_STAT)) == (READY_STAT | SEEK_STAT | DRQ_STAT))
			break;
	}

	port_read(HD_DATA(hd->channel), buf, count * SECTOR_SIZE / 2);
}

/*
 @brief 阻塞方式写硬盘
 @param hd 硬盘
 @param lba 要写的起始地址
 @param count 写多少扇区
 @param buf 写的数据
 */
void hd_write_sector(disk_t *hd, uint32_t lba, uint8_t count, uint8_t *buf) {
	uint8_t status = 0;
	while(1) {
		status = in_byte(HD_STATUS(hd->channel));
		if((status & BUSY_STAT) != BUSY_STAT)
			break;
	}

	out_param(hd, lba, count);
	out_cmd(hd->channel, CMD_WRITE);

	while(1) {
		status = in_byte(HD_STATUS(hd->channel));
		if((status & (READY_STAT | SEEK_STAT | DRQ_STAT)) == (READY_STAT | SEEK_STAT | DRQ_STAT))
			break;
	}

	port_write(HD_DATA(hd->channel), buf, count * SECTOR_SIZE / 2);
}

void hd_handler(uint32_t gs, uint32_t fs, uint32_t es, uint32_t ds, uint32_t edi, uint32_t esi,
	uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax,
	uint32_t irq_no) {
	uint8_t channel_no = irq_no - 0x20 - 14;
	hd_channel_t *channel = &channels[channel_no];

	INFO("channel : %s\r", channel_no ? "ata1" : "ata0")
	if(channel->expecting_intrrupt) {
		channel->expecting_intrrupt = false;
		semaphore_up(&channel->disk_done);

		in_byte(HD_STATUS(channel)); //清中断
	}
}

static bool hd_wait(disk_t *hd) {
	uint32_t time = 30 * 1000;
	uint8_t status = 0;

	while(time > 0) {
		status = in_byte(HD_STATUS(hd->channel));
		if((status & (READY_STAT | SEEK_STAT | DRQ_STAT)) == (READY_STAT | SEEK_STAT | DRQ_STAT))
			return true;
		else {
			sleep_ms(100);
			time -= 100;
		}
	}

	return false;
}

/*
 @param 中断方式读取硬盘
 @param hd 硬盘
 @param lba 要读的起始地址
 @param count 读多少扇区
 @param buf 读到哪
 */
void hd_read(disk_t *hd, uint32_t lba, uint32_t count, uint8_t *buf) {
	lock_acquire(&hd->channel->lock);

	uint32_t sector_number = 0;
	while(count) {
		if(count > 255)
			sector_number = 255;
		else
			sector_number = count;

		out_param(hd, lba, sector_number);
		out_cmd(hd->channel, CMD_READ);

		semaphore_down(&hd->channel->disk_done); //等待硬盘将数据读到硬盘缓冲区中

		/* 硬盘准备好数据后,产生中断,中断中释放信号量,继续执行代码 */
		if(!hd_wait(hd))
			PANIC("read disk error\r\n");

		port_read(HD_DATA(hd->channel), buf, sector_number * SECTOR_SIZE / 2);

		buf += sector_number * SECTOR_SIZE;
		lba += sector_number;
		count -= sector_number;
	}

	lock_release(&hd->channel->lock);
}

/*
 @brief 中断方式写硬盘
 @param hd 硬盘
 @param lba 要写的起始地址
 @param count 写多少扇区
 @param buf 写的数据
 */
void hd_write(disk_t *hd, uint32_t lba, uint32_t count, uint8_t *buf) {
	lock_acquire(&hd->channel->lock);

	uint32_t sector_number = 0;
	while(count) {
		if(count > 255)
			sector_number = 255;
		else
			sector_number = count;

		out_param(hd, lba, sector_number);
		out_cmd(hd->channel, CMD_WRITE);

		if(!hd_wait(hd))
			PANIC("write disk error\r\n");

		port_write(HD_DATA(hd->channel), buf, sector_number * SECTOR_SIZE / 2);

		semaphore_down(&hd->channel->disk_done); //等待硬盘将数据从硬盘缓冲区写入硬盘

		/* 硬盘写完数据后,产生中断,中断中释放信号量,继续执行代码 */
		buf += sector_number * SECTOR_SIZE;
		lba += sector_number;
		count -= sector_number;
	}

	lock_release(&hd->channel->lock);
}