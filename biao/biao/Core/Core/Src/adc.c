/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
#include "adc.h"
#include "stm32l1xx_hal.h"  // 确保包含HAL库头文件
#include "stm32l1xx_hal_adc.h"  // 确保包含ADC特定头文件
#include <string.h>  // 添加memset函数的头文件

/* USER CODE BEGIN 0 */
/**
  * @brief  ADC1初始化函数（适配STM32L152，PA5通道）
  * @param  None
  * @retval None
  */
void MX_ADC1_Init(void)
{
    ADC_HandleTypeDef hadc1;
    ADC_ChannelConfTypeDef sConfig = {0};

    // 初始化ADC句柄结构体
    memset(&hadc1, 0, sizeof(hadc1));

    // 1. ADC基础配置（不变）
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfDiscConversion = 1;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    
    // 确保ADC未处于活动状态
    __HAL_ADC_DISABLE(&hadc1);
    HAL_Delay(1);
    
    // STM32L1不支持Overrun字段，移除该行
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    // 2. 关键修改：配置ADC通道为PA5（通道5）
    sConfig.Channel = ADC_CHANNEL_5;  // PA5对应ADC通道5
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_192CYCLES; // 采样时间可根据需求调整
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    
    // 验证初始化成功
    if (HAL_ADC_Start(&hadc1) == HAL_OK) {
        HAL_ADC_Stop(&hadc1);  // 立即停止，因为我们只进行初始化
    }
}

/**
  * @brief  ADC MSP初始化（底层硬件配置，适配PA5）
  * @param  hadc: ADC句柄
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hadc->Instance==ADC1)
    {
        // 使能ADC1和GPIOA时钟（不变）
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // 关键修改：配置PA5为ADC输入
        GPIO_InitStruct.Pin = GPIO_PIN_5;  // 引脚改为PA5
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
/* USER CODE END 0 */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
