#ifndef _AT24C64_H_
#define _AT24C64_H_

#include "main.h"

/* AT24C64 I2C地址定义 */
#define AT24C64_ADDR_WRITE  0xA0    // 写地址
#define AT24C64_ADDR_READ   0xA1    // 读地址

/* AT24C64参数定义 */
#define AT24C64_PAGE_SIZE   32      // 页大小32字节
#define AT24C64_MAX_ADDR    8192    // 总容量8KB

/**
 * @brief AT24C64初始化
 */
void AT24C64_Init(void);

/**
 * @brief 写一个字节到AT24C64
 * @param addr 写入地址 (0-8191)
 * @param data 要写入的数据
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_WriteByte(uint16_t addr, uint8_t data);

/**
 * @brief 从AT24C64读一个字节
 * @param addr 读取地址 (0-8191)
 * @param data 读取数据的指针
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_ReadByte(uint16_t addr, uint8_t *data);

/**
 * @brief 页写入(最多32字节)
 * @param addr 起始地址
 * @param pData 数据指针
 * @param len 数据长度
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_WritePage(uint16_t addr, uint8_t *pData, uint16_t len);

/**
 * @brief 连续读取多个字节
 * @param addr 起始地址
 * @param pData 数据缓冲区指针
 * @param len 读取长度
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_ReadBytes(uint16_t addr, uint8_t *pData, uint16_t len);

/**
 * @brief 检测AT24C64是否正常
 * @return 0-正常, 1-异常
 */
uint8_t AT24C64_Check(void);

#endif
