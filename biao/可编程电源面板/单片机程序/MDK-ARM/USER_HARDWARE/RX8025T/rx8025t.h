#ifndef _RX8025T_H_
#define _RX8025T_H_

#include "main.h"

/* RX8025T I2C地址定义 */
#define RX8025T_ADDR_WRITE  0x64    // 写地址
#define RX8025T_ADDR_READ   0x65    // 读地址

/* RX8025T寄存器地址定义 */
#define RX8025T_REG_SEC     0x00    // 秒寄存器
#define RX8025T_REG_MIN     0x01    // 分寄存器
#define RX8025T_REG_HOUR    0x02    // 时寄存器
#define RX8025T_REG_WEEK    0x03    // 星期寄存器
#define RX8025T_REG_DAY     0x04    // 日寄存器
#define RX8025T_REG_MONTH   0x05    // 月寄存器
#define RX8025T_REG_YEAR    0x06    // 年寄存器
#define RX8025T_REG_OFFSET  0x07    // 偏移寄存器
#define RX8025T_REG_FLAG    0x0E    // 标志寄存器
#define RX8025T_REG_CTRL    0x0F    // 控制寄存器

/* 时间结构体定义 */
typedef struct {
    uint8_t year;       // 年 (0-99)
    uint8_t month;      // 月 (1-12)
    uint8_t day;        // 日 (1-31)
    uint8_t week;       // 星期 (0-6)
    uint8_t hour;       // 时 (0-23)
    uint8_t minute;     // 分 (0-59)
    uint8_t second;     // 秒 (0-59)
} RX8025T_Time;

/**
 * @brief RX8025T初始化
 */
void RX8025T_Init(void);

/**
 * @brief 写RX8025T寄存器
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_WriteReg(uint8_t reg, uint8_t data);

/**
 * @brief 读RX8025T寄存器
 * @param reg 寄存器地址
 * @param data 读取数据的指针
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_ReadReg(uint8_t reg, uint8_t *data);

/**
 * @brief 设置RTC时间
 * @param time 时间结构体指针
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_SetTime(RX8025T_Time *time);

/**
 * @brief 获取RTC时间
 * @param time 时间结构体指针
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_GetTime(RX8025T_Time *time);

/**
 * @brief BCD码转十进制
 * @param bcd BCD码
 * @return 十进制数
 */
uint8_t BCD_To_DEC(uint8_t bcd);

/**
 * @brief 十进制转BCD码
 * @param dec 十进制数
 * @return BCD码
 */
uint8_t DEC_To_BCD(uint8_t dec);

#endif
