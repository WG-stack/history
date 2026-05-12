/**
  ******************************************************************************
  * @file           : lcd_display.h
  * @brief          : 段码屏LCD显示模块头文件
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  * @attention
  * 
  * LCD配置：
  * - COM1, COM2, COM3, COM4 (4个COM口)
  * - SIG00-SIG26 (27个段信号)
  * - 使用STM32L1自带的LCD控制器
  * 
  ******************************************************************************
  */

#ifndef __LCD_DISPLAY_H
#define __LCD_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lcd.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
// LCD符号定义
#define LCD_SYMBOL_BATTERY      0   // 电池符号 'lo-batt' SEG17 COM0
#define LCD_SYMBOL_UNIT_MM      1   // 单位mm SEG16 COM0
#define LCD_SYMBOL_UNIT_INCH    2   // 单位inch 'in' SEG15 COM3
#define LCD_SYMBOL_NEGATIVE     3   // 负号 'sign' SEG0 COM2
#define LCD_SYMBOL_POSITIVE     4   // 正号 'positive' SEG0 COM3
#define LCD_SYMBOL_FWD          5   // 前进 'fwd' SEG0 COM1
#define LCD_SYMBOL_REV          6   // 后退 'rev' SEG0 COM0
#define LCD_SYMBOL_DOT_1        7   // 小数点1 (数字1后)
#define LCD_SYMBOL_DOT_2        8   // 小数点2 (数字2后)
#define LCD_SYMBOL_DOT_3        9   // 小数点3 (数字3后)
#define LCD_SYMBOL_DOT_4        10  // 小数点4 (数字4后)
#define LCD_SYMBOL_DOT_5        11  // 小数点5 (数字5后)

// SEG12-SEG25的符号定义
#define LCD_SYMBOL_SET          12  // 'set' SEG12 COM4
#define LCD_SYMBOL_OK           13  // 'ok' SEG13 COM1
#define LCD_SYMBOL_NG           14  // 'NG' SEG13 COM2
#define LCD_SYMBOL_TOL          15  // 'tol' SEG13 COM3
#define LCD_SYMBOL_REL          16  // 'rel' SEG13 COM4
#define LCD_SYMBOL_MAX          17  // 'max' SEG14 COM1
#define LCD_SYMBOL_TIR          18  // 'TIR' SEG14 COM2
#define LCD_SYMBOL_MIN          19  // 'min' SEG14 COM3
#define LCD_SYMBOL_HOLD         20  // 'hold' SEG14 COM4
#define LCD_SYMBOL_INS1         21  // 'ins1' SEG15 COM1
#define LCD_SYMBOL_INU1         22  // 'inu1' SEG15 COM2
#define LCD_SYMBOL_IN_P         23  // 'in-p' SEG15 COM3
#define LCD_SYMBOL_MM_P         24  // 'mm-p' SEG16 COM2
#define LCD_SYMBOL_MMST         25  // 'mmst' SEG16 COM3
#define LCD_SYMBOL_MMUT         26  // 'mmut' SEG16 COM4

// SEG17 电源和电池相关符号
#define LCD_SYMBOL_PS_PP        27  // 'ps++' SEG17 COM2
#define LCD_SYMBOL_PS_P15       28  // 'ps+15' SEG17 COM3
#define LCD_SYMBOL_PS_0         29  // 'ps0' SEG17 COM4

// SEG18-SEG25 电源档位符号 (ps+14 到 ps-15)
#define LCD_SYMBOL_PS_P14       30  // 'ps+14' SEG18 COM1
#define LCD_SYMBOL_PS_P13       31  // 'ps+13' SEG18 COM2
#define LCD_SYMBOL_PS_P12       32  // 'ps+12' SEG18 COM3
#define LCD_SYMBOL_PS_P11       33  // 'ps+11' SEG18 COM4
#define LCD_SYMBOL_PS_P10       34  // 'ps+10' SEG19 COM1
#define LCD_SYMBOL_PS_P9        35  // 'ps+9' SEG19 COM2
#define LCD_SYMBOL_PS_P8        36  // 'ps+8' SEG19 COM3
#define LCD_SYMBOL_PS_P7        37  // 'ps+7' SEG19 COM4
#define LCD_SYMBOL_PS_P6        38  // 'ps+6' SEG20 COM1
#define LCD_SYMBOL_PS_P5        39  // 'ps+5' SEG20 COM2
#define LCD_SYMBOL_PS_P4        40  // 'ps+4' SEG20 COM3
#define LCD_SYMBOL_PS_P3        41  // 'ps+3' SEG20 COM4
#define LCD_SYMBOL_PS_P2        42  // 'ps+2' SEG21 COM1
#define LCD_SYMBOL_PS_P1        43  // 'ps+1' SEG21 COM2
#define LCD_SYMBOL_PS_N1        44  // 'ps-1' SEG21 COM3
#define LCD_SYMBOL_PS_N2        45  // 'ps-2' SEG21 COM4
#define LCD_SYMBOL_PS_N3        46  // 'ps-3' SEG22 COM1
#define LCD_SYMBOL_PS_N4        47  // 'ps-4' SEG22 COM2
#define LCD_SYMBOL_PS_N5        48  // 'ps-5' SEG22 COM3
#define LCD_SYMBOL_PS_N6        49  // 'ps-6' SEG22 COM4
#define LCD_SYMBOL_PS_N7        50  // 'ps-7' SEG23 COM1
#define LCD_SYMBOL_PS_N8        51  // 'ps-8' SEG23 COM2
#define LCD_SYMBOL_PS_N9        52  // 'ps-9' SEG23 COM3
#define LCD_SYMBOL_PS_N10       53  // 'ps-10' SEG23 COM4
#define LCD_SYMBOL_PS_N11       54  // 'ps-11' SEG24 COM1
#define LCD_SYMBOL_PS_N12       55  // 'ps-12' SEG24 COM2
#define LCD_SYMBOL_PS_N13       56  // 'ps-13' SEG24 COM3
#define LCD_SYMBOL_PS_N14       57  // 'ps-14' SEG24 COM4
#define LCD_SYMBOL_PS_NN        58  // 'ps--' SEG25 COM1
#define LCD_SYMBOL_PS_N15       59  // 'ps-15' SEG25 COM2

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void LCD_Display_Init(void);
void LCD_Display_Clear(void);
void LCD_Display_Number(float number, uint8_t decimal_places);
void LCD_Display_String(char *str);
void LCD_Display_Symbol(uint8_t symbol, uint8_t state);
void LCD_Display_BatteryIcon(uint8_t show);
void LCD_Display_Update(void);

// 低层段码控制函数
void LCD_Write_Segment(uint8_t com, uint8_t seg, uint8_t state);
void LCD_Write_Digit(uint8_t position, uint8_t digit);
void LCD_Display_Dot(uint8_t position, uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_DISPLAY_H */
