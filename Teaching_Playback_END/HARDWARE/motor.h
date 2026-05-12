/**
 * @file    motor.h
 * @brief   TB6612FNG 双路直流电机驱动
 *
 * 硬件连接:
 *   PWMA -> PB8 (TIM4_CH3)    左电机速度
 *   AIN1 -> PB0               左电机方向1
 *   AIN2 -> PB1               左电机方向2
 *   PWMB -> PB9 (TIM4_CH4)    右电机速度
 *   BIN1 -> PB10              右电机方向1
 *   BIN2 -> PB11              右电机方向2
 *   STBY -> PB2               使能 (HIGH=工作, LOW=待机)
 *
 * TIM4: PSC=71, ARR=999 -> 1kHz PWM, 占空比 0~999
 */

#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "main.h"

/* PWM最大值 (需与TIM4的ARR一致) */
#define MOTOR_PWM_MAX    999

/**
 * @brief 初始化电机驱动 (启动PWM、使能STBY)
 */
void Motor_Init(void);

/**
 * @brief 设置左电机速度
 * @param speed  -MOTOR_PWM_MAX ~ +MOTOR_PWM_MAX，正值前进，负值后退
 */
void Motor_SetLeft(int16_t speed);

/**
 * @brief 设置右电机速度
 * @param speed  -MOTOR_PWM_MAX ~ +MOTOR_PWM_MAX，正值前进，负值后退
 */
void Motor_SetRight(int16_t speed);

/**
 * @brief 同时设置两路电机速度
 */
void Motor_SetSpeed(int16_t left_speed, int16_t right_speed);

/**
 * @brief 停止 (惰行，高阻抗，轮子可自由转动，用于示教推动阶段)
 */
void Motor_Stop(void);

/**
 * @brief 抱闸制动 (两端短接)
 */
void Motor_Brake(void);

/**
 * @brief 使能/禁用驱动芯片 (STBY引脚)
 * @param en  1=使能, 0=待机
 */
void Motor_Enable(uint8_t en);

#endif /* __MOTOR_H__ */
