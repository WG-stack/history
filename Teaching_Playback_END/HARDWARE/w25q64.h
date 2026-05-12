/**
 * @file    w25q64.h
 * @brief   W25Q64 SPI NOR Flash 驱动 (8MB)
 *
 * 硬件连接 (SPI2):
 *   CS   -> PB12 (软件控制)
 *   SCK  -> PB13
 *   MISO -> PB14
 *   MOSI -> PB15
 *
 * 芯片规格:
 *   容量: 64Mbit = 8MB
 *   页大小: 256 Bytes
 *   扇区大小: 4KB (16页/扇区)
 *   总页数: 32768
 *   JEDEC ID: 0xEF 0x40 0x17
 *
 * 轨迹数据存储布局:
 *   扇区0 (0x000000): Flash头部 (16字节)
 *   扇区1~N (0x001000起): 轨迹点数据
 *
 * 每个轨迹点 (TrajectoryPoint_t) = 4字节:
 *   int16_t left_delta  + int16_t right_delta
 * 最多可存储 (8MB - 4KB) / 4 = ~2,096,640 个点
 * 以10ms采样率 ≈ 5.8小时, 远超实际需求
 */

#ifndef __W25Q64_H__
#define __W25Q64_H__

#include "main.h"

#define W25Q64_PAGE_SIZE    256U
#define W25Q64_SECTOR_SIZE  4096U
#define W25Q64_TOTAL_SIZE   (8UL * 1024UL * 1024UL)

/**
 * @brief  初始化W25Q64, 验证JEDEC ID
 * @return 1=成功, 0=ID不匹配 (检查接线或芯片)
 */
uint8_t W25Q64_Init(void);

/**
 * @brief  从指定地址连续读取数据
 * @param  addr  起始地址 (0 ~ 0x7FFFFF)
 * @param  buf   接收缓冲区
 * @param  len   读取字节数
 */
void W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len);

/**
 * @brief  写入一页 (最多256字节, 地址须页对齐)
 * @note   写入前对应区域必须已擦除 (0xFF)
 * @param  addr  页起始地址 (须为256的倍数)
 * @param  buf   待写入数据
 * @param  len   字节数 (1~256)
 */
void W25Q64_WritePage(uint32_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief  擦除一个扇区 (4KB), 约60ms
 * @param  addr  扇区内任意地址 (自动对齐到4KB边界)
 */
void W25Q64_EraseSector(uint32_t addr);

/**
 * @brief  全片擦除, 约20~100秒 (一般不用)
 */
void W25Q64_EraseChip(void);

/**
 * @brief  读取芯片忙状态
 * @return 1=忙, 0=空闲
 */
uint8_t W25Q64_IsBusy(void);

/**
 * @brief  等待芯片操作完成 (阻塞)
 */
void W25Q64_WaitDone(void);

#endif /* __W25Q64_H__ */
