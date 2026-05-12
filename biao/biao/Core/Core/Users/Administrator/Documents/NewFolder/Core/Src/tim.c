/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "stm32l1xx_hal.h"  // 添加HAL库头文件
#include <string.h>         // 添加string.h用于memset函数
#include "stm32l1xx_hal_tim.h"  // 添加定时器相关的头文件

/* USER CODE BEGIN 0 */

/* Global variable for TIM7 interrupt counter - moved to global_variables.c */
/* uint32_t tim7_interrupt_count = 0; */

/* USER CODE END 0 */

// 声明htim7变量（如果不存在的话）
TIM_HandleTypeDef htim7;  // 修改：将extern改为定义，因为这个变量需要在此文件中定义

// 添加htim2变量的定义
TIM_HandleTypeDef htim2;  // 定义htim2变量

/* TIM2 init function */
void MX_TIM2_Init(void)
{
  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA); // 提前使能GPIOA时钟，规范配置

  /* USER CODE BEGIN TIM2_Init 1 */
  /* USER CODE END TIM2_Init 1 */
  // 定时器基础配置：保留原频率核心参数 PSC=0，调整ARR为偶数实现精准50%占空比
  TIM_InitStruct.Prescaler = 0;                    // 预分频0，无分频
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP; // 向上计数
  TIM_InitStruct.Autoreload = 6;                  // 保持原参数，频率≈1.4545MHz
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM2, &TIM_InitStruct);
  LL_TIM_EnableARRPreload(TIM2);                   // 使能ARR预装载
  LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL); // 内部时钟16MHz

  // 核心：配置CH1为【硬件自动翻转模式】（OC Toggle）
  LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH1);
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_TOGGLE; // 关键：改为比较翻转模式
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE; // 使能通道输出（必须开启）
  TIM_OC_InitStruct.CompareValue = 3;              // 匹配值=ARR/2，实现50%占空比
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH; // 初始极性高
  LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
  LL_TIM_OC_DisableFast(TIM2, LL_TIM_CHANNEL_CH1); // 关闭快速输出，常规配置

  LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM2);

  /* USER CODE BEGIN TIM2_Init 2 */
  /* USER CODE END TIM2_Init 2 */
  // PA0配置为TIM2_CH1复用输出（AF1），与原配置一致，无需修改
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;   // 复用模式（核心，保留定时器通道）
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH; // 超高速，匹配高频翻转
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;        // PA0-TIM2_CH1 复用映射AF1
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // 启动定时器+使能通道输出，硬件自动开始翻转
  LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);
  LL_TIM_EnableCounter(TIM2);
  
  // 确保定时器已启动后再继续
  // 添加短暂延迟确保定时器已稳定运行
  for(volatile int i = 0; i < 1000; i++);
}

void MX_TIM7_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM7);

  /* TIM7 interrupt Init */
  HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM7_IRQn);

  /* USER CODE BEGIN TIM7_Init 1 */
  /* USER CODE END TIM7_Init 1 */

  // Configure TIM7 for 1kHz update rate (1ms interrupt)
  TIM_InitStruct.Prescaler = 16000 - 1;           // 16MHz / 16000 = 1kHz
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 1000 - 1;           // 1kHz / 1000 = 1ms interrupt
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM7, &TIM_InitStruct);
  LL_TIM_SetClockSource(TIM7, LL_TIM_CLOCKSOURCE_INTERNAL);

  /* USER CODE BEGIN TIM7_Init 2 */
  // Enable TIM7 update interrupt
  LL_TIM_EnableIT_UPDATE(TIM7);
  /* USER CODE END TIM7_Init 2 */

  // Start TIM7 counter
  LL_TIM_EnableCounter(TIM7);
}

/* USER CODE BEGIN 3 */

/* USER CODE END 1 */