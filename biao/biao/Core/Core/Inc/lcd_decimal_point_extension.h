/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd_decimal_point_extension.h
  * @brief   Extension header for LCD display with enhanced decimal point support
  *          Based on the requirement: 小数点并不占位，是依复于7段码的，比如‘11.111’小数点在第4位后，所以第4位的小数点段显示
  ******************************************************************************
  * @attention
  *
  * This extension adds enhanced decimal point handling functionality to the LCD display
  * according to the requirement that decimal points are tied to specific digit positions
  * rather than being separate characters.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LCD_DECIMAL_POINT_EXTENSION_H__
#define __LCD_DECIMAL_POINT_EXTENSION_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Include the unified LCD display header */
#include "lcd_display.h"

/**
  * @brief LCD显示带小数点的字符串
  * @param str: 要显示的字符串，例如"11.111"表示在第2位数字后显示小数点
  * @retval None
  * 
  * 此函数实现了用户需求：小数点并不占位，是依复于7段码的
  * 例如：对于'11.111'，小数点在第4位后，所以第4位的小数点段显示
  */
void LCDDisplay_ShowStringWithDecimalPoints(const char* str);

/**
  * @brief LCD显示整数形式的数字（推荐使用）
  * @param integer_number: 整数形式的数值，例如12345表示123.45
  * @param decimal_places: 小数位数或小数点位置（从右数第几位后显示小数点），例如2表示在百分位后显示小数点
  * @retval None
  * 
  * 示例：显示123.45则调用 LCDDisplay_ShowIntegerNumber_New(12345, 2)
  * 表示在整数12345的从右数第2位后显示小数点，即显示为123.45
  */
void LCDDisplay_ShowIntegerNumber_New(int32_t integer_number, uint8_t decimal_places);

/**
  * @brief LCD显示数字 - 基于整数的版本，避免浮点数精度问题（推荐使用）
  * @param number: 整数形式的数值，例如12345表示123.45
  * @param decimal_places: 小数位数或小数点位置（从右数第几位后显示小数点），例如2表示在百分位后显示小数点
  * @retval None
  * 
  * 示例：显示123.45则调用 LCDDisplay_ShowNumber_IntegerBased(12345, 2)
  * 表示在整数12345的从右数第2位后显示小数点，即显示为123.45
  */
void LCDDisplay_ShowNumber_IntegerBased(int32_t number, uint8_t decimal_places);

/**
  * @brief 设置指定位置的小数点
  * @param position: 位置 (LCD_POSITION_1 到 LCD_POSITION_6)
  * @param enabled: 1为启用小数点，0为禁用
  * @retval None
  */
void LCDDisplay_SetDecimalPointForPosition(LCD_Position_TypeDef position, uint8_t enabled);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_DECIMAL_POINT_EXTENSION_H__ */