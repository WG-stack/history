/**
 * @file    encoder.h
 * @brief   增量式光电编码器读取
 *
 * 硬件连接:
 *   左电机编码器: TIM2 (PA0=CH1/A相, PA1=CH2/B相) — 4倍频计数
 *   右电机编码器: TIM3 (PA6=CH1/A相, PA7=CH2/B相) — 4倍频计数
 *
 * 使用方法:
 *   Encoder_Init() 后每隔固定周期调用 Encoder_GetLeftDelta() / Encoder_GetRightDelta()
 *   可获取该周期内的脉冲增量 (带符号, 正=前进, 负=后退)
 */

#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "main.h"

/**
 * @brief 初始化编码器 (启动TIM2/TIM3编码器模式)
 */
void Encoder_Init(void);

/**
 * @brief 读取左轮编码器增量并清零基准
 * @return 自上次调用以来的脉冲数 (有符号)
 */
int16_t Encoder_GetLeftDelta(void);

/**
 * @brief 读取右轮编码器增量并清零基准
 * @return 自上次调用以来的脉冲数 (有符号)
 */
int16_t Encoder_GetRightDelta(void);

/**
 * @brief 获取左轮累积脉冲数 (上电以来)
 */
int32_t Encoder_GetLeftTotal(void);

/**
 * @brief 获取右轮累积脉冲数 (上电以来)
 */
int32_t Encoder_GetRightTotal(void);

/**
 * @brief 复位累积计数 (不影响增量读取)
 */
void Encoder_ResetTotal(void);

#endif /* __ENCODER_H__ */
