#ifndef _BCD_OUTPUT_H_
#define _BCD_OUTPUT_H_

#include "main.h"

/**
 * @brief BCD输出初始化
 */
void BCD_Init(void);

/**
 * @brief 设置BCD输出值(7位)
 * @param value 输出值 (bit0-bit6对应BCD_OUT0-BCD_OUT6)
 */
void BCD_SetOutput(uint8_t value);

/**
 * @brief 设置单个BCD输出位
 * @param bit 位号 (0-6)
 * @param state 状态 (0-低电平, 1-高电平)
 */
void BCD_SetBit(uint8_t bit, uint8_t state);

/**
 * @brief 获取当前BCD输出值
 * @return 当前输出值
 */
uint8_t BCD_GetOutput(void);

/**
 * @brief 使能BCD公共端
 */
void BCD_COM_Enable(void);

/**
 * @brief 禁用BCD公共端
 */
void BCD_COM_Disable(void);

#endif
