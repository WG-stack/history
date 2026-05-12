/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd.c
  * @brief   LCD液晶显示屏配置文件
  *          此文件包含LCD实例的初始化代码和GPIO配置
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
 *
  * 本软件遵循可在软件组件根目录中的LICENSE文件中找到的条款进行许可
  * 如果没有随软件提供LICENSE文件，则按原样提供
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* 包含必要的头文件 ------------------------------------------------------------------*/
#include "lcd.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* LCD句柄定义 */
LCD_HandleTypeDef hlcd;

/**
  * @brief LCD初始化函数
  * @param 无
  * @retval 无
  * @note 此函数用于配置STM32L1xx微控制器的LCD外设参数，包括分频器、占空比、偏置电压等设置
  */
void MX_LCD_Init(void)
{
  /* USER CODE BEGIN LCD_Init 0 */

  /* USER CODE END LCD_Init 0 */

  /* USER CODE BEGIN LCD_Init 1 */

  /* USER CODE END LCD_Init 1 */
  
  /* 配置LCD句柄参数 */
  hlcd.Instance = LCD;                                    // LCD外设实例
  hlcd.Init.Prescaler = LCD_PRESCALER_1;                  // 预分频器设置为1
  hlcd.Init.Divider = LCD_DIVIDER_16;                     // 分频系数设置为16
  hlcd.Init.Duty = LCD_DUTY_1_4;                          // LCD占空比设置为1/4
  hlcd.Init.Bias = LCD_BIAS_1_3;                          // LCD偏置设置为1/3
  hlcd.Init.VoltageSource = LCD_VOLTAGESOURCE_INTERNAL;   // 使用内部电压源
 hlcd.Init.Contrast = LCD_CONTRASTLEVEL_5;               // 对比度级别设置为3
  hlcd.Init.DeadTime = LCD_DEADTIME_1;                    // 死区时间设置为1个脉冲周期
  hlcd.Init.PulseOnDuration = LCD_PULSEONDURATION_3;      // 脉冲持续时间设置为3个时钟周期
  hlcd.Init.MuxSegment = LCD_MUXSEGMENT_ENABLE;           // 启用多路复用段功能
  hlcd.Init.BlinkMode = LCD_BLINKMODE_OFF;                // 关闭闪烁模式
  hlcd.Init.BlinkFrequency = LCD_BLINKFREQUENCY_DIV128;   // 闪烁频率设置为LCD时钟的1/128
  
  /* 初始化LCD外设 */
  if (HAL_LCD_Init(&hlcd) != HAL_OK)
  {
    Error_Handler();  // 如果初始化失败，进入错误处理函数
  }
  /* USER CODE BEGIN LCD_Init 2 */

  /* USER CODE END LCD_Init 2 */
}

/**
  * @brief LCD MSP初始化函数
  * @param lcdHandle: LCD句柄指针
  * @retval 无
  * @note 此函数用于初始化LCD外设的底层GPIO和时钟，包括使能相关时钟和配置GPIO为复用功能
  */
void HAL_LCD_MspInit(LCD_HandleTypeDef* lcdHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(lcdHandle->Instance==LCD)
  {
  /* USER CODE BEGIN LCD_MspInit 0 */

  /* USER CODE END LCD_MspInit 0 */
    
    /* 使能LCD外设时钟 */
    __HAL_RCC_LCD_CLK_ENABLE();

    /* 使能相关GPIO端口时钟 */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /**LCD GPIO配置说明
    PC0     ------> LCD_SEG18    // LCD段18
    PC1     ------> LCD_SEG19    // LCD段19
    PC2     ------> LCD_SEG20    // LCD段20
    PC3     ------> LCD_SEG21    // LCD段21
    PA1     ------> LCD_SEG0     // LCD段0
    PA2     ------> LCD_SEG1     // LCD段1
    PA3     ------> LCD_SEG2     // LCD段2
    PA6     ------> LCD_SEG3     // LCD段3
    PA7     ------> LCD_SEG4     // LCD段4
    PC4     ------> LCD_SEG22    // LCD段22
    PC5     ------> LCD_SEG23    // LCD段23
    PB0     ------> LCD_SEG5     // LCD段5
    PB1     ------> LCD_SEG6     // LCD段6
    PB10    ------> LCD_SEG10    // LCD段10
    PB11    ------> LCD_SEG11    // LCD段11
    PB12    ------> LCD_SEG12    // LCD段12
    PB13    ------> LCD_SEG13    // LCD段13
    PB14    ------> LCD_SEG14    // LCD段14
    PB15    ------> LCD_SEG15    // LCD段15
    PC6     ------> LCD_SEG24    // LCD段24
    PC7     ------> LCD_SEG25    // LCD段25
    PC8     ------> LCD_SEG26    // LCD段26
    PA8     ------> LCD_COM0     // LCD公共端0
    PA9     ------> LCD_COM1     // LCD公共端1
    PA10    ------> LCD_COM2     // LCD公共端2
    PA15    ------> LCD_SEG17    // LCD段17
    PB3     ------> LCD_SEG7     // LCD段7
    PB4     ------> LCD_SEG8     // LCD段8
    PB5     ------> LCD_SEG9     // LCD段9
    PB8     ------> LCD_SEG16    // LCD段16
    PB9     ------> LCD_COM3     // LCD公共端3
    */
    
    /* 配置GPIOC上的LCD段引脚 */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;              // 复用推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;                  // 无上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;         // 低速
    GPIO_InitStruct.Alternate = GPIO_AF11_LCD;           // 复用功能LCD
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 配置GPIOA上的LCD段和公共端引脚 */
    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;              // 复用推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;                  // 无上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;         // 低速
    GPIO_InitStruct.Alternate = GPIO_AF11_LCD;           // 复用功能LCD
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 配置GPIOB上的LCD段和公共端引脚 */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8
                          |GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;              // 复用推挽输出模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;                  // 无上下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;         // 低速
    GPIO_InitStruct.Alternate = GPIO_AF11_LCD;           // 复用功能LCD
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN LCD_MspInit 1 */

  /* USER CODE END LCD_MspInit 1 */
  }
}

/**
  * @brief LCD MSP反初始化函数
  * @param lcdHandle: LCD句柄指针
  * @retval 无
  * @note 此函数用于释放LCD外设的底层资源，包括禁用时钟和GPIO引脚
  */
void HAL_LCD_MspDeInit(LCD_HandleTypeDef* lcdHandle)
{
  if(lcdHandle->Instance==LCD)
  {
  /* USER CODE BEGIN LCD_MspDeInit 0 */

  /* USER CODE END LCD_MspDeInit 0 */
    /* 禁用LCD外设时钟 */
    __HAL_RCC_LCD_CLK_DISABLE();

    /**LCD GPIO配置撤销
    PC0     ------> LCD_SEG18
    PC1     ------> LCD_SEG19
    PC2     ------> LCD_SEG20
    PC3     ------> LCD_SEG21
    PA1     ------> LCD_SEG0
    PA2     ------> LCD_SEG1
    PA3     ------> LCD_SEG2
    PA6     ------> LCD_SEG3
    PA7     ------> LCD_SEG4
    PC4     ------> LCD_SEG22
    PC5     ------> LCD_SEG23
    PB0     ------> LCD_SEG5
    PB1     ------> LCD_SEG6
    PB10     ------> LCD_SEG10
    PB11     ------> LCD_SEG11
    PB12     ------> LCD_SEG12
    PB13     ------> LCD_SEG13
    PB14     ------> LCD_SEG14
    PB15     ------> LCD_SEG15
    PC6     ------> LCD_SEG24
    PC7     ------> LCD_SEG25
    PC8     ------> LCD_SEG26
    PA8     ------> LCD_COM0
    PA9     ------> LCD_COM1
    PA10     ------> LCD_COM2
    PA15     ------> LCD_SEG17
    PB3     ------> LCD_SEG7
    PB4     ------> LCD_SEG8
    PB5     ------> LCD_SEG9
    PB8     ------> LCD_SEG16
    PB9     ------> LCD_COM3
    */
    
    /* 反初始化GPIOC上的LCD引脚 */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_8);

    /* 反初始化GPIOA上的LCD引脚 */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_15);

    /* 反初始化GPIOB上的LCD引脚 */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_8
                          |GPIO_PIN_9);

  /* USER CODE BEGIN LCD_MspDeInit 1 */

  /* USER CODE END LCD_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
