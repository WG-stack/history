/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd_driver.h
  * @brief   This file provides code for the LCD driver module header
  *          implementing segment LCD control for the displacement sensor project
  ******************************************************************************
  * @attention
  *
  * This module implements the LCD driver functions as specified in the project
  * requirements, including digit display, symbol control, and power management.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LCD_DRIVER_H__
#define __LCD_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lcd.h"
#include "data_processing.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Enum for LCD segments */
typedef enum {
    LCD_SEG_REV = 0,      // COM1/SEG0
    LCD_SEG_FWD,          // COM2/SEG0
    LCD_SEG_SIGN,         // COM3/SEG0
    LCD_SEG_POSITIVE,     // COM4/SEG0
    
    LCD_SEG_1F,           // COM1/SEG1
    LCD_SEG_1G,           // COM2/SEG1
    LCD_SEG_1E,           // COM3/SEG1
    LCD_SEG_1D,           // COM4/SEG1
    
    LCD_SEG_1A,           // COM1/SEG2
    LCD_SEG_1B,           // COM2/SEG2
    LCD_SEG_1C,           // COM3/SEG2
    LCD_SEG_P1,           // COM4/SEG2 - Decimal point for digit 1
    
    LCD_SEG_2F,           // COM1/SEG3
    LCD_SEG_2G,           // COM2/SEG3
    LCD_SEG_2E,           // COM3/SEG3
    LCD_SEG_2D,           // COM4/SEG3
    
    LCD_SEG_2A,           // COM1/SEG4
    LCD_SEG_2B,           // COM2/SEG4
    LCD_SEG_2C,           // COM3/SEG4
    LCD_SEG_P2,           // COM4/SEG4 - Decimal point for digit 2
    
    LCD_SEG_3F,           // COM1/SEG5
    LCD_SEG_3G,           // COM2/SEG5
    LCD_SEG_3E,           // COM3/SEG5
    LCD_SEG_3D,           // COM4/SEG5
    
    LCD_SEG_3A,           // COM1/SEG6
    LCD_SEG_3B,           // COM2/SEG6
    LCD_SEG_3C,           // COM3/SEG6
    LCD_SEG_P3,           // COM4/SEG6 - Decimal point for digit 3
    
    LCD_SEG_4F,           // COM1/SEG7
    LCD_SEG_4G,           // COM2/SEG7
    LCD_SEG_4E,           // COM3/SEG7
    LCD_SEG_4D,           // COM4/SEG7
    
    LCD_SEG_4A,           // COM1/SEG8
    LCD_SEG_4B,           // COM2/SEG8
    LCD_SEG_4C,           // COM3/SEG8
    LCD_SEG_P4,           // COM4/SEG8 - Decimal point for digit 4
    
    LCD_SEG_5F,           // COM1/SEG9
    LCD_SEG_5G,           // COM2/SEG9
    LCD_SEG_5E,           // COM3/SEG9
    LCD_SEG_5D,           // COM4/SEG9
    
    LCD_SEG_5A,           // COM1/SEG10
    LCD_SEG_5B,           // COM2/SEG10
    LCD_SEG_5C,           // COM3/SEG10
    LCD_SEG_P5,           // COM4/SEG10 - Decimal point for digit 5
    
    LCD_SEG_6F,           // COM1/SEG11
    LCD_SEG_6G,           // COM2/SEG11
    LCD_SEG_6E,           // COM3/SEG11
    LCD_SEG_6D,           // COM4/SEG11
    
    LCD_SEG_6A,           // COM1/SEG12
    LCD_SEG_6B,           // COM2/SEG12
    LCD_SEG_6C,           // COM3/SEG12
    LCD_SEG_SET,          // COM4/SEG12 - SET indicator (was P6)
    
    LCD_SEG_OK,           // COM1/SEG13
    LCD_SEG_NG,           // COM2/SEG13
    LCD_SEG_TOL,          // COM3/SEG13
    LCD_SEG_REL,          // COM4/SEG13
    
    LCD_SEG_MAX,          // COM1/SEG14
    LCD_SEG_TIR,          // COM2/SEG14
    LCD_SEG_MIN,          // COM3/SEG14
    LCD_SEG_HOLD,         // COM4/SEG14
    
    LCD_SEG_INS1,         // COM1/SEG15
    LCD_SEG_INU1,         // COM2/SEG15
    LCD_SEG_IN_P,         // COM3/SEG15
    LCD_SEG_IN,           // COM4/SEG15
    
    LCD_SEG_MM,           // COM1/SEG16
    LCD_SEG_MM_P,         // COM2/SEG16
    LCD_SEG_MMST,         // COM3/SEG16
    LCD_SEG_MMUT,         // COM4/SEG16
    
    LCD_SEG_LO_BATT,      // COM1/SEG17
    LCD_SEG_PS_PP,        // COM2/SEG17 (ps++)
    LCD_SEG_PS_P15,       // COM3/SEG17 (ps+15)
    LCD_SEG_PS_0,         // COM4/SEG17 (ps0)
    
    LCD_SEG_PS_P14,       // COM1/SEG18 (ps+14)
    LCD_SEG_PS_P13,       // COM2/SEG18 (ps+13)
    LCD_SEG_PS_P12,       // COM3/SEG18 (ps+12)
    LCD_SEG_PS_P11,       // COM4/SEG18 (ps+11)
    
    LCD_SEG_PS_P10,       // COM1/SEG19 (ps+10)
    LCD_SEG_PS_P9,        // COM2/SEG19 (ps+9)
    LCD_SEG_PS_P8,        // COM3/SEG19 (ps+8)
    LCD_SEG_PS_P7,        // COM4/SEG19 (ps+7)
    
    LCD_SEG_PS_P6,        // COM1/SEG20 (ps+6)
    LCD_SEG_PS_P5,        // COM2/SEG20 (ps+5)
    LCD_SEG_PS_P4,        // COM3/SEG20 (ps+4)
    LCD_SEG_PS_P3,        // COM4/SEG20 (ps+3)
    
    LCD_SEG_PS_P2,        // COM1/SEG21 (ps+2)
    LCD_SEG_PS_P1,        // COM2/SEG21 (ps+1)
    LCD_SEG_PS_N1,        // COM3/SEG21 (ps-1)
    LCD_SEG_PS_N2,        // COM4/SEG21 (ps-2)
    
    LCD_SEG_PS_N3,        // COM1/SEG22 (ps-3)
    LCD_SEG_PS_N4,        // COM2/SEG22 (ps-4)
    LCD_SEG_PS_N5,        // COM3/SEG22 (ps-5)
    LCD_SEG_PS_N6,        // COM4/SEG22 (ps-6)
    
    LCD_SEG_PS_N7,        // COM1/SEG23 (ps-7)
    LCD_SEG_PS_N8,        // COM2/SEG23 (ps-8)
    LCD_SEG_PS_N9,        // COM3/SEG23 (ps-9)
    LCD_SEG_PS_N10,       // COM4/SEG23 (ps-10)
    
    LCD_SEG_PS_N11,       // COM1/SEG24 (ps-11)
    LCD_SEG_PS_N12,       // COM2/SEG24 (ps-12)
    LCD_SEG_PS_N13,       // COM3/SEG24 (ps-13)
    LCD_SEG_PS_N14,       // COM4/SEG24 (ps-14)
    
    LCD_SEG_PS_MM,        // COM1/SEG25 (ps--)
    LCD_SEG_PS_N15,       // COM2/SEG25 (ps-15)
    LCD_SEG_MAX_ENUM      // Must be last
} lcd_segment_t;

/* Structure for mapping LCD segment enums to COM and SEG values */
typedef struct {
    uint8_t com;  // COM line (0-3)
    uint8_t seg;  // SEG line (0-25)
} lcd_com_seg_map_t;

/* Structure for digit segment mapping */
typedef struct {
    lcd_segment_t seg_a;   // Segment A
    lcd_segment_t seg_b;   // Segment B
    lcd_segment_t seg_c;   // Segment C
    lcd_segment_t seg_d;   // Segment D
    lcd_segment_t seg_e;   // Segment E
    lcd_segment_t seg_f;   // Segment F
    lcd_segment_t seg_g;   // Segment G
    lcd_segment_t seg_dp;  // Decimal Point
} lcd_digit_segments_t;

/* USER CODE BEGIN 1 */
/************************ 新增：解决 #136 错误的关键定义 ************************/
/**
 * @brief 扩展传感器数据结构体（兼容 lcd_driver.c 调用）
 * 补充 lcd_driver.c 中用到但未定义的字段
 */
typedef struct {
    uint8_t     digits[6];      // 6位数字数组，对应显示的每一位
    uint8_t     unit_metric;    // 单位标识：1=公制(mm)，0=英制(in)
    uint8_t     last_digit;     // 最后一位小数（用于PS指针显示）
    // 继承 SensorData_TypeDef 的原有字段
    uint8_t     is_valid;       // 数据有效标志
    int32_t     value;          // 测量原始值
    uint8_t     sign;           // 符号位（与 sign_value 对应）
    uint8_t     unit;           // 单位（与 unit_metric 对应）
    uint8_t     mode;           // 测量模式
} SensorData_Ext_TypeDef;

/**
 * @brief 全局LCD显示缓冲区映射表
 * 用于快速查找每个数字对应的段码
 */
extern const lcd_digit_segments_t lcd_digit_map[6];

/************************ 新增：函数声明（解决隐式警告） ************************/
/**
 * @brief 获取图标对应的COM/SEG引脚
 * @param icon  图标类型（lcd_segment_t）
 * @param com   输出参数：COM引脚号
 * @param seg   输出参数：SEG引脚号
 * @retval 1=成功，0=无效图标
 */
uint8_t lcd_get_com_seg_from_icon(lcd_segment_t icon, uint8_t* com, uint8_t* seg);

/* USER CODE END 1 */

/** @addtogroup LCD_Driver
  * @{
  */

/* Exported functions --------------------------------------------------------*/
// ... 以下内容保持原文件不变 ...
void lcd_driver_init(void);
void lcd_clear_all(void);
void lcd_update_display(void);
void lcd_set_segment(uint8_t com, uint8_t seg);
void lcd_clear_segment(uint8_t com, uint8_t seg);
void lcd_set_ram_bit(uint8_t com, uint8_t seg, uint8_t state);
void lcd_write_ram(uint8_t ram_idx, uint32_t data);
uint32_t lcd_read_ram(uint8_t ram_idx);
uint32_t lcd_read_hardware_ram(uint8_t ram_idx);
void lcd_refresh_display(void);
void lcd_force_update_display(void);
void lcd_display_number(float number, uint8_t decimal_places);
void lcd_display_digit_with_dp(uint8_t position, uint8_t digit, uint8_t show_dp);
void lcd_display_digit(uint8_t position, uint8_t digit);
void lcd_display_char(uint8_t position, char ch);
void lcd_display_char_segments_with_dp(uint8_t position, uint8_t segments, uint8_t show_dp);
void lcd_display_char_segments(uint8_t position, uint8_t segments);
void lcd_show_symbol(lcd_segment_t symbol, uint8_t state);
void lcd_show_multiple_symbols(lcd_segment_t *symbols, uint8_t *states, uint8_t count);
void lcd_show_max_min_tir_symbols(uint8_t max_active, uint8_t min_active, uint8_t tir_active);
void lcd_show_ok_ng_symbols(uint8_t ok_active, uint8_t ng_active);
void lcd_show_unit_symbols(uint8_t mm_active, uint8_t in_active);
void lcd_show_tolerance_symbol(uint8_t active);
void lcd_show_relative_symbol(uint8_t active);
void lcd_show_hold_symbol(uint8_t active);
void lcd_show_low_battery_symbol(uint8_t active);
void lcd_show_plus_minus_symbols(int8_t value);
void lcd_set_symbol_state(lcd_segment_t symbol, uint8_t state);
void lcd_clear_all_symbols(void);
void lcd_show_ps_symbols(uint8_t last_digit, uint8_t sign);
void lcd_display_sensor_data_from_struct(const SensorData_TypeDef *sensor_data);
void lcd_update_from_sensor_data(void);
void lcd_display_string(const char* str);

/* USER CODE BEGIN Prototypes */

/* Include SensorData_TypeDef definition */
#include "global_variables.h"  // 包含SensorData_TypeDef的定义
void lcd_set_ram_bit(uint8_t com, uint8_t seg, uint8_t state);
void LCD_PrepareForStopMode(void);
void LCD_RestoreAfterStopMode(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __LCD_DRIVER_H__ */


