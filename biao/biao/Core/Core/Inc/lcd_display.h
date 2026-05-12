/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd_display.h
  * @brief   Header for lcd_display.c module
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LCD_DISPLAY_H
#define __LCD_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "measurement_state_machine.h"

/* USER CODE BEGIN Includes */
#include "global_variables.h"  // 包含SensorData_TypeDef的定义
#include "lcd_driver.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/**
  * @brief LCD显示位置枚举
  */
typedef enum {
    LCD_POSITION_1 = 0,
    LCD_POSITION_2,
    LCD_POSITION_3,
    LCD_POSITION_4,
    LCD_POSITION_5,
    LCD_POSITION_6,
    LCD_POSITION_MAX
} LCD_Position_TypeDef;

/**
  * @brief LCD显示字符枚举
  */
typedef enum {
    LCD_CHAR_0 = 0,
    LCD_CHAR_1,
    LCD_CHAR_2,
    LCD_CHAR_3,
    LCD_CHAR_4,
    LCD_CHAR_5,
    LCD_CHAR_6,
    LCD_CHAR_7,
    LCD_CHAR_8,
    LCD_CHAR_9,
    LCD_CHAR_A,
    LCD_CHAR_B,
    LCD_CHAR_C,
    LCD_CHAR_D,
    LCD_CHAR_E,
    LCD_CHAR_F,
    LCD_CHAR_G,
    LCD_CHAR_H,
    LCD_CHAR_I,
    LCD_CHAR_J,
    LCD_CHAR_L,
    LCD_CHAR_O,
    LCD_CHAR_P,
    LCD_CHAR_S,
    LCD_CHAR_T,
    LCD_CHAR_U,
    LCD_CHAR_Y,
    LCD_CHAR_Z,
    LCD_CHAR_NEGATIVE,
    LCD_CHAR_DASH,
    LCD_CHAR_BLANK,
    LCD_CHAR_MAX
} LCD_Char_TypeDef;

/**
  * @brief LCD图标枚举
  */
typedef enum {
    LCD_ICON_MM = 0,
    LCD_ICON_IN,
    LCD_ICON_REL,
    LCD_ICON_MAX,
    LCD_ICON_MIN,
    LCD_ICON_TIR,
    LCD_ICON_TOL,
    LCD_ICON_OK,
    LCD_ICON_NG,
    LCD_ICON_PS_PLUS_PLUS,
    LCD_ICON_PS_PLUS_15,
    LCD_ICON_PS_PLUS_14,
    LCD_ICON_PS_PLUS_13,
    LCD_ICON_PS_PLUS_12,
    LCD_ICON_PS_PLUS_11,
    LCD_ICON_PS_PLUS_10,
    LCD_ICON_PS_PLUS_9,
    LCD_ICON_PS_PLUS_8,
    LCD_ICON_PS_PLUS_7,
    LCD_ICON_PS_PLUS_6,
    LCD_ICON_PS_PLUS_5,
    LCD_ICON_PS_PLUS_4,
    LCD_ICON_PS_PLUS_3,
    LCD_ICON_PS_PLUS_2,
    LCD_ICON_PS_PLUS_1,
    LCD_ICON_PS_ZERO,
    LCD_ICON_PS_MINUS_1,
    LCD_ICON_PS_MINUS_2,
    LCD_ICON_PS_MINUS_3,
    LCD_ICON_PS_MINUS_4,
    LCD_ICON_PS_MINUS_5,
    LCD_ICON_PS_MINUS_6,
    LCD_ICON_PS_MINUS_7,
    LCD_ICON_PS_MINUS_8,
    LCD_ICON_PS_MINUS_9,
    LCD_ICON_PS_MINUS_10,
    LCD_ICON_PS_MINUS_11,
    LCD_ICON_PS_MINUS_12,
    LCD_ICON_PS_MINUS_13,
    LCD_ICON_PS_MINUS_14,
    LCD_ICON_PS_MINUS_15,
    LCD_ICON_PS_MINUS_MINUS,
    LCD_ICON_MAX_NUM
} LCD_Icon_TypeDef;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* LCD显示状态机相关数据结构定义 */
typedef struct {
    uint8_t is_metric;               // 是否公制单位 (1=MM, 0=IN) - 使用布尔值表示
    uint8_t mode;                    // 显示模式
    uint8_t resolution;              // 分辨率
    uint8_t decimal_position;        // 小数点位置
    uint8_t icons_status[LCD_ICON_MAX_NUM]; // 图标状态数组
} LCD_DisplayState_TypeDef;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/**
  * @brief  初始化LCD显示驱动
  * @param  None
  * @retval None
  */
void LCDDisplay_Init_New(void);

/**
  * @brief  清除LCD显示
  * @param  None
  * @retval None
  */
void LCDDisplay_Clear_New(void);

/**
  * @brief  在指定位置显示字符
  * @param  ch: 要显示的字符
  * @param  pos: 显示位置
  * @retval None
  */
void LCDDisplay_ShowChar_New(LCD_Char_TypeDef ch, LCD_Position_TypeDef pos);

/**
  * @brief  显示字符串
  * @param  str: 要显示的字符串
 * @retval None
  */
void LCDDisplay_ShowString_New(const char* str);

/**
  * @brief  显示带小数点的字符串
  * @param  str: 要显示的字符串
  * @retval None
  */
void LCDDisplay_ShowStringWithDecimalPoints(const char* str);

/**
  * @brief  显示或隐藏指定图标
  * @param  icon: 图标类型
  * @param  enable: 1-显示 0-隐藏
  * @retval None
  */
void LCDDisplay_ShowIcon_New(LCD_Icon_TypeDef icon, uint8_t enable);

/**
  * @brief  隐藏指定图标
  * @param  icon: 图标类型
  * @retval None
  */
void LCDDisplay_ClearIcon_New(LCD_Icon_TypeDef icon);

/**
  * @brief  获取指定图标状态
  * @param  icon: 图标类型
  * @retval 1-显示 0-隐藏
  */
uint8_t LCDDisplay_GetIconStatus_New(LCD_Icon_TypeDef icon);


/**
  * @brief  在指定位置显示特殊符号
  * @param  symbol: 符号字符串
  * @param  pos: 显示位置
  * @retval None
  */
void LCDDisplay_ShowSymbol_New(const char* symbol, LCD_Position_TypeDef pos);

/**
  * @brief  更新LCD显示（将缓冲区内容写入硬件）
  * @param  None
  * @retval None
  */
void LCDDisplay_Update_New(void);

/**
  * @brief  字符转段码
  * @param  ch: 要转换的字符
  * @retval 对应的段码值
  */
uint32_t LCDDisplay_CharToSegments_New(LCD_Char_TypeDef ch);

/**
  * @brief  统一显示接口 - 输入处理后的数据自动完成所有显示
  * @param  processed_data: 处理后的传感器数据结构体
  * @retval None
  */
void LCDDisplay_ShowProcessedData(SensorData_TypeDef processed_data);

/* LCD显示状态机相关函数 */
void LCD_DisplayState_Init(void);
void LCD_DisplayState_SetMetric(uint8_t is_metric);
void LCD_DisplayState_SetUnit(uint8_t unit);  // 添加SetUnit函数声明
void LCD_DisplayState_SetMode(uint8_t mode);
void LCD_DisplayState_SetResolution(uint8_t resolution);
void LCD_DisplayState_SetDecimalPosition(uint8_t position);
void LCD_DisplayState_SetIcon(LCD_Icon_TypeDef icon, uint8_t enable);
uint8_t LCD_DisplayState_GetIconStatus(LCD_Icon_TypeDef icon);
uint8_t LCD_DisplayState_GetIsMetric(void);  // 获取是否公制
LCD_DisplayState_TypeDef* LCD_DisplayState_GetCurrent(void);

/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif


#endif /* __LCD_DISPLAY_H */
