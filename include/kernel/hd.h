/*
 bochsrc 配置
 ata0: enabled=true, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
 ata0-master: type=disk, path="hd20M.img", mode=flat
 ata0-slave: type=none
 ata1: enabled=true, ioaddr1=0x170, ioaddr2=0x370, irq=15
 ata1-master: type=none
 ata1-slave: type=none
 
 有两个ata通道,每个通道支持两个盘
 ata0: 使用io端口寄存器0x1F0-0x1F7 中断号 14
 ata1: 使用io端口寄存器0x170-0x177 中断号 15
 级联,需要额外开启中断号 2
 */

#ifndef __HD_H__
#define __HD_H__

#include "../stdint.h"
#include "./sync.h"

/* 分区结构 */
typedef struct hd_partition {

}hd_partition_t;

typedef struct hd_channel hd_channel_t;

/* 硬盘结构 */
typedef struct disk {
	char name[8];
	hd_channel_t *channel; //硬盘归属那个通道
	uint8_t dev_no; //主/从盘
	hd_partition_t prim_parts[4]; //4个主分区
	hd_partition_t logic_parts[8]; //逻辑分区
}disk_t;

/* ata通道结构 */
typedef struct hd_channel{
	char name[8];
	uint16_t port_base; //io端口寄存器基址
	uint8_t irq_no; //中断号
	lock_t lock;
	semaphore_t disk_done;
	bool expecting_intrrupt; //中断控制
	disk_t disk[2];
}hd_channel_t;
extern hd_channel_t channels[2];

extern void hd_init(void);
extern void hd_read_sector(disk_t *hd, uint32_t lba, uint8_t count, void *buf);
extern void hd_write_sector(disk_t *hd, uint32_t lba, uint8_t count, uint8_t *buf);

#endif