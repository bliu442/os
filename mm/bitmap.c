/* 使用位图管理内存 一位映射一页内存 */

#include "../include/kernel/bitmap.h"
#include "../include/string.h"
#include "../include/kernel/debug.h"

/* 初始化内存池 */
void bitmap_init(bitmap_t *bitmap) {
	memset(bitmap->map, bitmap_unused, bitmap->bitmap_byte_len);
}

/*
 @brief 查看index处内存页是否被使用
 @param bitmap 内存池首地址
 @param index 索引
 @retval bitmap_state_t
 */
bitmap_state_t bitmap_scan(bitmap_t *bitmap, uint32_t index) {
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;
	bitmap_state_t state = bitmap_used;

	state = (bitmap->map[byte_index] & (1 << bit_index)) ? bitmap_used : bitmap_unused;
	return state;
}

/*
 @brief 申请连续count个内存页
 @param bitmap 内存池首地址
 @param count 连续内存页个数
 @retval 成功 连续内存页地址下标
 失败 -1
 */
int bitmap_continuous_scan(bitmap_t *bitmap, uint32_t count) {
	uint32_t byte_index = 0;

	while(0xFF == bitmap->map[byte_index] && (byte_index < bitmap->bitmap_byte_len)) //mark 逐字节比较
		byte_index++;

	ASSERT(byte_index < bitmap->bitmap_byte_len);
	if(byte_index == bitmap->bitmap_byte_len)
		return -1;
	
	uint32_t bit_index = 0;
	while((bitmap_used << bit_index) & bitmap->map[byte_index]) {
		bit_index++;
	}

	int bit_index_start = byte_index * 8 + bit_index;
	if(1 == count)
		return bit_index_start;
	
	uint32_t bit_left = (bitmap->bitmap_byte_len * 8 - bit_index_start);
	uint32_t next_bit = bit_index_start + 1;
	uint32_t find_count = 1;

	bit_index_start = -1;
	while(bit_left-- > 0) {
		if(bitmap_unused == bitmap_scan(bitmap, next_bit))
			find_count++;
		else
			find_count = 0;

		if(count == find_count) {
			bit_index_start = next_bit - count + 1;
			break;
		}
		next_bit++;
	}

	return bit_index_start;
}

/*
 @brief 将位图index位设置为value
 @param bitmap 内存池首地址
 @param index 索引
 @param value bitmap_state_t
 */
void bitmap_set(bitmap_t *bitmap, uint32_t index, uint8_t value) {
	uint32_t byte_index = index / 8;
	uint32_t bit_index = index % 8;

	if(value)
		bitmap->map[byte_index] |= (1 << bit_index);
	else
		bitmap->map[byte_index] &= ~(1 << bit_index);
}