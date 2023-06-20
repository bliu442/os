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

 分区
 mbr引导扇区 446B(引导代码) + 64B(分区表) + 2B(0x55aa) 固定0磁道0柱面1扇区
 分区结构 16B mbr只能存放4个分区表(主分区)
 一个主分区用作拓展分区(3个主分区+1个拓展分区) sector_start = sector_offect + 0
 拓展分区MBR(EBR 只看分区表) 存放1个逻辑分区+1个子拓展分区(1) sector_start = sector_offect + 拓展分区sector_start
 子拓展分区(1)MBR(EBR) 存放1个逻辑分区+1个子拓展分区(2) sector_start = sector_offect + 拓展分区sector_start
 ...
 */

#ifndef __HD_H__
#define __HD_H__

#include "../stdint.h"
#include "./sync.h"
#include "./bitmap.h"

#define SECTOR_SIZE 512

typedef struct super_block super_block_t;

/* 分区表结构 */
typedef struct partition {
	uint8_t bootable; //是否可引导 0x80
	uint8_t start_head;
	uint8_t start_sector;
	uint8_t start_cylinder;
	uint8_t type; //分区类型 主分区 拓展分区
	uint8_t end_head;
	uint8_t end_sector;
	uint8_t end_cylinder;
	uint32_t sector_offset;
	uint32_t sector_number; //总扇区数
}partition_t;

typedef struct mbr_sector {
	uint8_t code[446];
	partition_t partition[4];
	uint16_t magic;
}__attribute__((packed)) mbr_sector_t;

typedef struct disk disk_t;
/* 分区结构 */
typedef struct hd_partition {
	char name[10];
	uint32_t sector_start; //分区表中存放的是分区偏移地址 转换为起始扇区存储 sector_start = sector_offect + 基准
	uint32_t sector_number;
	disk_t *disk; //分区所属硬盘
	list_item_t part_list_item;
	super_block_t *sb; //分区的超级块
	bitmap_t block_bitmap; //块位图
	bitmap_t inode_bitmap; //inode位图
	list_t open_inodes; //分区打开的inode队列
}hd_partition_t;

typedef struct hd_channel hd_channel_t;
/* 硬盘结构 */
typedef struct disk {
	char name[8];
	hd_channel_t *channel; //硬盘归属那个通道
	uint8_t dev_no; //主/从盘
	hd_partition_t prim_parts[4]; //4个主分区/3个主分区与一个拓展分区
	hd_partition_t logic_parts[8]; //逻辑分区

	char info[512]; //全部info信息 搞个联合体 懒得写了
	uint16_t cylinder; //柱面数
	uint16_t head; //磁头数
	uint16_t track_sectors; //磁道扇区数
	char number[20+ 1]; //硬盘序列号
	uint16_t buffer_type; //缓冲区类型 (3 写入缓存)
	uint16_t buffer_size; //缓冲区大小 (byte 淦)
	char model[40 + 1]; //硬盘型号
	int sectors; //扇区数
}disk_t;

/* ata通道结构 */
typedef struct hd_channel{
	char name[8];
	uint16_t port_base; //io端口寄存器基址
	uint8_t irq_no; //中断号
	lock_t lock; //用于申请channel ata
	semaphore_t disk_done; //用于控制硬盘数据传输
	bool expecting_intrrupt; //中断控制
	disk_t disk[2];
}hd_channel_t;
extern hd_channel_t channels[2];
extern const uint8_t channel_number;

extern list_t partition_list;

extern void hd_init(void);
extern void hd_read_sector(disk_t *hd, uint32_t lba, uint8_t count, void *buf);
extern void hd_write_sector(disk_t *hd, uint32_t lba, uint8_t count, uint8_t *buf);
extern void hd_read(disk_t *hd, uint32_t lba, uint32_t count, uint8_t *buf);
extern void hd_write(disk_t *hd, uint32_t lba, uint32_t count, uint8_t *buf);

#endif