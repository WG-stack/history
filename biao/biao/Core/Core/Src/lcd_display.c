/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd_display.c
  * @brief   LCD display driver module implementation file based on the provided segment table
 ******************************************************************************
  * @attention
  *
  * 工作流程：
  * 1. 初始化LCD显示驱动，配置硬件和显示缓冲区
  * 2. 根据输入的传感器数据结构体，解析数值、符号、单位、模式等信息
  * 3. 将数字、图标、指针等元素渲染到显示缓冲区
  * 4. 更新LCD硬件显示内容
  *
  * 实现函数：
  * - LCDDisplay_Init_New(): LCD显示驱动初始化函数
  * - LCDDisplay_Clear_New(): LCD清屏函数
  * - LCDDisplay_ShowChar_New(): 在指定位置显示字符函数
  * - LCDDisplay_ShowString_New(): 显示字符串函数
  * - LCDDisplay_ShowStringWithDecimalPoints(): 显示带小数点的字符串函数
  * - LCDDisplay_ShowIcon_New(): 显示或隐藏指定图标函数
  * - LCDDisplay_ClearIcon_New(): 隐藏指定图标函数
  * - LCDDisplay_GetIconStatus_New(): 获取指定图标状态函数
  * - LCDDisplay_ShowAnalogPointer_New(): 显示模拟指针函数
  * - LCDDisplay_ShowSymbol_New(): 在指定位置显示特殊符号函数
  * - LCDDisplay_Update_New(): 更新LCD显示函数
  * - LCDDisplay_CharToSegments_New(): 字符转段码函数
  * - LCDDisplay_ShowProcessedData(): 统一显示接口函数，根据处理后的数据自动完成所有显示
  *
  * This file implements LCD display functionality based on the segment table specification.
  * 它提供了对STM32微控制器的LCD外设的高级抽象接口。
  * 主要功能包括：
  * - 数字和字符显示
  * - 小数点控制
  * - 图标显示
  * - 模拟指针显示
  * - 批量显示更新机制
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "lcd_display.h"
#include "lcd_driver.h"
#include "main.h"
#include "global_variables.h"  // 添加全局变量头文件，用于访问key_fsm
#include "measurement_state_machine.h"  // 添加测量状态机支持
#include <string.h>
#include <stdlib.h>
/* Suppress unused variable warnings */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#elif defined(__ICCARM__)
#pragma diag_suppress=Pe177
#elif defined(__CC_ARM)
#pragma diag_suppress=177
#endif

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* COM/SEG位掩码定义 */
#define LCD_COM1_MASK           0x01
#define LCD_COM2_MASK           0x02
#define LCD_COM3_MASK           0x04
#define LCD_COM4_MASK           0x08

/* 外部全局变量声明（用于获取测量状态） */
extern ZTJ_TypeDef measurement_state;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
// 存储当前显示内容的缓冲区
// lcd_display_buffer_new[0]: COM1的数据 (SEG0-SEG25)
// lcd_display_buffer_new[1]: COM2的数据 (SEG0-SEG25)
// lcd_display_buffer_new[2]: COM3的数据 (SEG0-SEG25)
// lcd_display_buffer_new[3]: COM4的数据 (SEG0-SEG25)
static uint32_t lcd_display_buffer_new[4];

/* 7段数码管段码定义 (a, b, c, d, e, f, g) */
static const uint8_t digit_patterns_new[10] = {
    0x3F, // 0: abcdef (0b00111111)
    0x06,  // 1: bc     (0b00000110)
    0x5B,  // 2: abged  (0b01011011)
    0x4F,  // 3: abgcd  (0b01001111)
    0x66,  // 4: fgbc   (0b0110)
    0x6D,  // 5: afgcd  (0b01101101)
    0x7D,  // 6: afebcd (0b01111101)
    0x07,  // 7: abc    (0b00000111)
    0x7F,  // 8: abcdefg(0b01111111)
    0x6F   // 9: abcdfg (0b01101111)
};

/* 记录每个位置的小数点状态 */
static uint8_t decimal_point_positions[6] __attribute__((unused)) = {0, 0, 0, 0, 0, 0};

// LCD显示状态机状态由外部全局变量管理
LCD_DisplayState_TypeDef lcd_display_state;  // 添加LCD显示状态机实例定义
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* 内部辅助函数声明 */
static void set_decimal_point_for_position(LCD_Position_TypeDef pos, uint8_t enabled);
static void lcd_set_ram_bit_internal(uint8_t com, uint8_t seg, uint8_t state);
// 新增：内部辅助函数 - 显示负号
static void lcd_show_negative_sign(uint8_t is_negative);
// 新增：内部辅助函数 - 显示单位图标
static void lcd_show_unit_icon(uint8_t is_metric);
// 新增：内部辅助函数 - 显示模式图标
static void lcd_show_mode_icon(MeasurementMode_TypeDef mode);
// 新增：内部辅助函数 - 显示公差图标
static void lcd_show_tolerance_icon(int32_t display_value);

/* 新增：统一显示接口声明 */
void LCDDisplay_ShowProcessedData(SensorData_TypeDef processed_data);

/* LCD显示状态机相关函数 */
void LCD_DisplayState_Init(void);
void LCD_DisplayState_SetUnit(uint8_t unit);
void LCD_DisplayState_SetMode(uint8_t mode);
void LCD_DisplayState_SetResolution(uint8_t resolution);
void LCD_DisplayState_SetDecimalPosition(uint8_t position);
void LCD_DisplayState_SetIcon(LCD_Icon_TypeDef icon, uint8_t enable);
uint8_t LCD_DisplayState_GetIconStatus(LCD_Icon_TypeDef icon);
LCD_DisplayState_TypeDef* LCD_DisplayState_GetCurrent(void);
/* USER CODE END PFP */

/* Public function prototypes ------------------------------------------------*/
/* USER CODE BEGIN PFP_PUBLIC */
/* USER CODE END PFP_PUBLIC */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void LCDDisplay_Init_New(void)
{
    // 初始化LCD硬件
    MX_LCD_Init();
    
    // 清除LCD显示缓存，准备新的显示内容
    LCDDisplay_Clear_New();
    
    // 初始化显示相关状态（默认关闭所有图标、设置默认显示参数）
    // 清除所有模式图标（REL/MAX/MIN/TIR/TOL）
    LCDDisplay_ClearIcon_New(LCD_ICON_REL);
    LCDDisplay_ClearIcon_New(LCD_ICON_MAX);
    LCDDisplay_ClearIcon_New(LCD_ICON_MIN);
    LCDDisplay_ClearIcon_New(LCD_ICON_TIR);
    LCDDisplay_ClearIcon_New(LCD_ICON_TOL);
    
    // 清除公差图标（OK/NG）
    LCDDisplay_ClearIcon_New(LCD_ICON_OK);
    LCDDisplay_ClearIcon_New(LCD_ICON_NG);
    
    // 清除指针图标（PS系列）- 不再需要，因为现在使用全局模拟指针值
    // PS0图标会在LCDDisplay_ShowProcessedData中根据模式自动处理
    LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // 初始化时always显示PS0图标
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_PLUS); // 清除PS++图标
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_MINUS); // 清除PS--图标
    // 清除所有PS+图标
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_1);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_2);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_3);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_4);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_5);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_6);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_7);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_8);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_9);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_10);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_11);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_12);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_13);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_14);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_15);
    // 清除所有PS-图标
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_1);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_2);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_3);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_4);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_5);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_6);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_7);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_8);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_9);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_10);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_11);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_12);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_13);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_14);
    LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_15);
    
    // 发起首次显示更新，确保初始化生效
    LCDDisplay_Update_New();
}

/**
  * @brief LCD清屏
  * @param None
  * @retval None
  */
void LCDDisplay_Clear_New(void)
{
    // 清除本地缓冲区
    for(int i = 0; i < 4; i++)
    {
        lcd_display_buffer_new[i] = 0;
    }
    
    // 清除小数点状态
    for(int i = 0; i < 6; i++)
    {
        decimal_point_positions[i] = 0;
    }
    // 直接清LCD硬件寄存器
    for(int i = 0; i < 8; i++) { // RAM寄存器 0, 2, 4, 6 用于COM0-COM3
        if(i == 0 || i == 2 || i == 4 || i == 6) {
            hlcd.Instance->RAM[i] = 0;
        }
    }
    
    // 发起显示更新请求
    HAL_LCD_UpdateDisplayRequest(&hlcd);
}

/**
  * @brief LCD显示字符
  * @param ch: 要显示的字符
  * @param pos: 显示位置
  * @retval None
  */
void LCDDisplay_ShowChar_New(LCD_Char_TypeDef ch, LCD_Position_TypeDef pos)
{
    // 根据位置获取对应的段码信息
    uint8_t seg_a, seg_b, seg_c, seg_d, seg_e, seg_f, seg_g;
    uint8_t com_a, com_b, com_c, com_d, com_e, com_f, com_g;
    
    switch(pos) {
        case LCD_POSITION_1:
            seg_a = 2; seg_b = 2; seg_c = 2; seg_d = 1; seg_e = 1; seg_f = 1; seg_g = 1;
            com_a = 0; com_b = 1; com_c = 2; com_d = 3; com_e = 2; com_f = 0; com_g = 1;
            break;
        case LCD_POSITION_2:
            seg_a = 4; seg_b = 4; seg_c = 4; seg_d = 3; seg_e = 3; seg_f = 3; seg_g = 3;
            com_a = 0; com_b = 1; com_c = 2; com_d = 3; com_e = 2; com_f = 0; com_g = 1;
            break;
        case LCD_POSITION_3:
            seg_a = 6; seg_b = 6; seg_c = 6; seg_d = 5; seg_e = 5; seg_f = 5; seg_g = 5;
            com_a = 0; com_b = 1; com_c = 2; com_d = 3; com_e = 2; com_f = 0; com_g = 1;
            break;
        case LCD_POSITION_4:
            seg_a = 8; seg_b = 8; seg_c = 8; seg_d = 7; seg_e = 7; seg_f = 7; seg_g = 7;
            com_a = 0; com_b = 1; com_c = 2; com_d = 3; com_e = 2; com_f = 0; com_g = 1;
            break;
        case LCD_POSITION_5:
            seg_a = 10; seg_b = 10; seg_c = 10; seg_d = 9; seg_e = 9; seg_f = 9; seg_g = 9;
            com_a = 0; com_b = 1; com_c = 2; com_d = 3; com_e = 2; com_f = 0; com_g = 1;
            break;
        case LCD_POSITION_6:
            seg_a = 12; seg_b = 12; seg_c = 12; seg_d = 11; seg_e = 11; seg_f = 11; seg_g = 11;
            com_a = 0; com_b = 1; com_c = 2; com_d = 3; com_e = 2; com_f = 0; com_g = 1;
            break;
        default:
            return;
    }
    
    // 获取数字的段码
    uint8_t segments = 0;
    if(ch <= LCD_CHAR_9) {
        segments = digit_patterns_new[ch - LCD_CHAR_0];
    } else if(ch == LCD_CHAR_NEGATIVE || ch == LCD_CHAR_DASH) {
        segments = 0x40; // g段，显示"-"
    } else if(ch == LCD_CHAR_BLANK) {
        segments = 0x00; // 不亮任何段
    } else if(ch >= LCD_CHAR_A && ch <= LCD_CHAR_Z) {
        switch(ch) {
            case LCD_CHAR_A: segments = 0x77; break;
            case LCD_CHAR_B: segments = 0x7C; break;
            case LCD_CHAR_C: segments = 0x39; break;
            case LCD_CHAR_D: segments = 0x5E; break;
            case LCD_CHAR_E: segments = 0x79; break;
            case LCD_CHAR_F: segments = 0x71; break;
            case LCD_CHAR_G: segments = 0x3D; break;
            case LCD_CHAR_H: segments = 0x76; break;
            case LCD_CHAR_I: segments = 0x06; break;
            case LCD_CHAR_J: segments = 0x0E; break;
            case LCD_CHAR_L: segments = 0x38; break;
            case LCD_CHAR_O: segments = 0x3F; break;
            case LCD_CHAR_P: segments = 0x73; break;
            case LCD_CHAR_S: segments = 0x6D; break;
            case LCD_CHAR_T: segments = 0x78; break;
            case LCD_CHAR_U: segments = 0x3E; break;
            case LCD_CHAR_Y: segments = 0x6E; break;
            case LCD_CHAR_Z: segments = 0x5B; break;
            default: segments = 0x00; break;
        }
    } else {
        segments = 0x00;
    }
    
    // 设置各段
    lcd_set_ram_bit_internal(com_a, seg_a, (segments >> 0) & 1); // a段
    lcd_set_ram_bit_internal(com_b, seg_b, (segments >> 1) & 1); // b段
    lcd_set_ram_bit_internal(com_c, seg_c, (segments >> 2) & 1); // c段
    lcd_set_ram_bit_internal(com_d, seg_d, (segments >> 3) & 1); // d段
    lcd_set_ram_bit_internal(com_e, seg_e, (segments >> 4) & 1); // e段
    lcd_set_ram_bit_internal(com_f, seg_f, (segments >> 5) & 1); // f段
    lcd_set_ram_bit_internal(com_g, seg_g, (segments >> 6) & 1); // g段
}
/**
  * @brief  根据图标类型获取对应的COM和SEG引脚
  * @param  icon: 要查询的LCD图标（来自 lcd_segment_t 枚举）
  * @param  com: 输出参数，存储图标对应的COM线（0-3）
  * @param  seg: 输出参数，存储图标对应的SEG线（0-25）
  * @retval 1=成功获取，0=无效图标
  * @note   引脚映射严格对应 lcd_driver.h 中的 lcd_segment_t 定义
  */
uint8_t lcd_get_com_seg_from_icon(lcd_segment_t icon, uint8_t* com, uint8_t* seg)
{
    // 校验输入参数
    if(com == NULL || seg == NULL) return 0;
    
    // 根据图标枚举，返回对应的COM/SEG引脚（参考 lcd_segment_t 定义）
    switch(icon)
    {
        // 符号类图标
        case LCD_SEG_SIGN:        *com = 2; *seg = 0;  break;
        case LCD_SEG_POSITIVE:    *com = 3; *seg = 0;  break;
        
        // 单位图标
        case LCD_SEG_MM:          *com = 0; *seg = 16; break;
        case LCD_SEG_IN:          *com = 3; *seg = 15; break;
        
        // 模式图标
        case LCD_SEG_REL:         *com = 3; *seg = 13; break;
        case LCD_SEG_MAX:         *com = 0; *seg = 14; break;
        case LCD_SEG_MIN:         *com = 2; *seg = 14; break;
        case LCD_SEG_TIR:         *com = 1; *seg = 14; break;
        case LCD_SEG_TOL:         *com = 2; *seg = 13; break;
        
        // 公差图标
        case LCD_SEG_OK:          *com = 0; *seg = 13; break;
        case LCD_SEG_NG:          *com = 1; *seg = 13; break;
        
        // 指针图标（PS系列）
        case LCD_SEG_PS_0:        *com = 3; *seg = 17; break;
        case LCD_SEG_PS_P1:       *com = 1; *seg = 21; break;
        case LCD_SEG_PS_P2:       *com = 0; *seg = 21; break;
        case LCD_SEG_PS_P3:       *com = 3; *seg = 20; break;
        case LCD_SEG_PS_P4:       *com = 2; *seg = 20; break;
        case LCD_SEG_PS_P5:       *com = 1; *seg = 20; break;
        case LCD_SEG_PS_P6:       *com = 0; *seg = 20; break;
        case LCD_SEG_PS_P7:       *com = 3; *seg = 19; break;
        case LCD_SEG_PS_P8:       *com = 2; *seg = 19; break;
        case LCD_SEG_PS_P9:       *com = 1; *seg = 19; break;
        case LCD_SEG_PS_P10:      *com = 0; *seg = 19; break;
        case LCD_SEG_PS_P11:      *com = 3; *seg = 18; break;
        case LCD_SEG_PS_P12:      *com = 2; *seg = 18; break;
        case LCD_SEG_PS_P13:      *com = 1; *seg = 18; break;
        case LCD_SEG_PS_P14:      *com = 0; *seg = 18; break;
        case LCD_SEG_PS_P15:      *com = 2; *seg = 17; break;
        case LCD_SEG_PS_PP:       *com = 1; *seg = 17; break;
        case LCD_SEG_PS_N1:       *com = 2; *seg = 21; break;
        case LCD_SEG_PS_N2:       *com = 3; *seg = 21; break;
        case LCD_SEG_PS_N3:       *com = 0; *seg = 22; break;
        case LCD_SEG_PS_N4:       *com = 1; *seg = 22; break;
        case LCD_SEG_PS_N5:       *com = 2; *seg = 22; break;
        case LCD_SEG_PS_N6:       *com = 3; *seg = 22; break;
        case LCD_SEG_PS_N7:       *com = 0; *seg = 23; break;
        case LCD_SEG_PS_N8:       *com = 1; *seg = 23; break;
        case LCD_SEG_PS_N9:       *com = 2; *seg = 23; break;
        case LCD_SEG_PS_N10:      *com = 3; *seg = 23; break;
        case LCD_SEG_PS_N11:      *com = 0; *seg = 24; break;
        case LCD_SEG_PS_N12:      *com = 1; *seg = 24; break;
        case LCD_SEG_PS_N13:      *com = 2; *seg = 24; break;
        case LCD_SEG_PS_N14:      *com = 3; *seg = 24; break;
        case LCD_SEG_PS_N15:      *com = 1; *seg = 25; break;
        case LCD_SEG_PS_MM:       *com = 0; *seg = 25; break;
        
        // 其他图标
        case LCD_SEG_HOLD:        *com = 3; *seg = 14; break;
        case LCD_SEG_LO_BATT:     *com = 0; *seg = 17; break;
        
        // 无效图标
        default: return 0;
    }
    
    return 1; // 成功获取
}
/**
  * @brief LCD显示字符串
  * @param str: 要显示的字符串
  * @retval None
  */
void LCDDisplay_ShowString_New(const char* str)
{
    if(str == NULL) return;
    
    LCDDisplay_Clear_New();
    
    // 解析输入字符串，识别数字和字符
    uint8_t char_idx = 0;  // 输入字符串索引
    uint8_t pos = 0;       // 显示位置 (0-5)
    
    while(str[char_idx] != '\0' && pos < 6) {
        if(str[char_idx] >= '0' && str[char_idx] <= '9') {
            LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(str[char_idx] - '0');
            LCDDisplay_ShowChar_New(ch, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] >= 'A' && str[char_idx] <= 'Z') {
            LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(str[char_idx] - 'A' + LCD_CHAR_A);
            LCDDisplay_ShowChar_New(ch, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] >= 'a' && str[char_idx] <= 'z') {
            LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(str[char_idx] - 'a' + LCD_CHAR_A);
            LCDDisplay_ShowChar_New(ch, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] == '-' && pos < 6){
            LCDDisplay_ShowChar_New(LCD_CHAR_DASH, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] == '.' && pos < 5){
            if(pos > 0) {
                set_decimal_point_for_position((LCD_Position_TypeDef)(pos - 1), 1);
            }
        }
        else if(pos < 6){
            LCDDisplay_ShowChar_New(LCD_CHAR_BLANK, (LCD_Position_TypeDef)pos);
            pos++;
        }
        
        char_idx++;
    }
}

/**
  * @brief LCD显示带小数点的字符串
  * @param str: 要显示的字符串
  * @retval None
  */
void LCDDisplay_ShowStringWithDecimalPoints(const char* str)
{
    if(str == NULL) return;
    
    LCDDisplay_Clear_New();
    
    // 解析输入字符串，识别数字和小数点
    uint8_t char_idx = 0;  // 输入字符串索引
    uint8_t pos = 0;       // 显示位置 (0-5)
    
    while(str[char_idx] != '\0' && pos < 6) {
        if(str[char_idx] >= '0' && str[char_idx] <= '9') {
            LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(str[char_idx] - '0');
            LCDDisplay_ShowChar_New(ch, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] >= 'A' && str[char_idx] <= 'Z') {
            LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(str[char_idx] - 'A' + LCD_CHAR_A);
            LCDDisplay_ShowChar_New(ch, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] >= 'a' && str[char_idx] <= 'z') {
            LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(str[char_idx] - 'a' + LCD_CHAR_A);
            LCDDisplay_ShowChar_New(ch, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] == '-' && pos < 6){
            LCDDisplay_ShowChar_New(LCD_CHAR_DASH, (LCD_Position_TypeDef)pos);
            pos++;
        }
        else if(str[char_idx] == '.' && pos < 6 && pos > 0){
            set_decimal_point_for_position((LCD_Position_TypeDef)(pos - 1), 1);
        }
        else if(pos < 6){
            LCDDisplay_ShowChar_New(LCD_CHAR_BLANK, (LCD_Position_TypeDef)pos);
            pos++;
        }
        
        char_idx++;
    }
}

/**
  * @brief LCD显示图标
  * @param icon: 图标类型
  * @param enable: 1为显示，0为清除
  * @retval None
  */
void LCDDisplay_ShowIcon_New(LCD_Icon_TypeDef icon, uint8_t enable)
{
    uint8_t com, seg;
    
    // 需要将 LCD_Icon_TypeDef 枚举转换为 lcd_segment_t 枚举
    lcd_segment_t converted_icon;
    switch(icon) {
        case LCD_ICON_MM: converted_icon = LCD_SEG_MM; break;
        case LCD_ICON_IN: converted_icon = LCD_SEG_IN; break;
        case LCD_ICON_REL: converted_icon = LCD_SEG_REL; break;
        case LCD_ICON_MAX: converted_icon = LCD_SEG_MAX; break;
        case LCD_ICON_MIN: converted_icon = LCD_SEG_MIN; break;
        case LCD_ICON_TIR: converted_icon = LCD_SEG_TIR; break;
        case LCD_ICON_TOL: converted_icon = LCD_SEG_TOL; break;
        case LCD_ICON_OK: converted_icon = LCD_SEG_OK; break;
        case LCD_ICON_NG: converted_icon = LCD_SEG_NG; break;
        case LCD_ICON_PS_PLUS_PLUS: converted_icon = LCD_SEG_PS_PP; break;
        case LCD_ICON_PS_PLUS_15: converted_icon = LCD_SEG_PS_P15; break;
        case LCD_ICON_PS_PLUS_14: converted_icon = LCD_SEG_PS_P14; break;
        case LCD_ICON_PS_PLUS_13: converted_icon = LCD_SEG_PS_P13; break;
        case LCD_ICON_PS_PLUS_12: converted_icon = LCD_SEG_PS_P12; break;
        case LCD_ICON_PS_PLUS_11: converted_icon = LCD_SEG_PS_P11; break;
        case LCD_ICON_PS_PLUS_10: converted_icon = LCD_SEG_PS_P10; break;
        case LCD_ICON_PS_PLUS_9: converted_icon = LCD_SEG_PS_P9; break;
        case LCD_ICON_PS_PLUS_8: converted_icon = LCD_SEG_PS_P8; break;
        case LCD_ICON_PS_PLUS_7: converted_icon = LCD_SEG_PS_P7; break;
        case LCD_ICON_PS_PLUS_6: converted_icon = LCD_SEG_PS_P6; break;
        case LCD_ICON_PS_PLUS_5: converted_icon = LCD_SEG_PS_P5; break;
        case LCD_ICON_PS_PLUS_4: converted_icon = LCD_SEG_PS_P4; break;
        case LCD_ICON_PS_PLUS_3: converted_icon = LCD_SEG_PS_P3; break;
        case LCD_ICON_PS_PLUS_2: converted_icon = LCD_SEG_PS_P2; break;
        case LCD_ICON_PS_PLUS_1: converted_icon = LCD_SEG_PS_P1; break;
        case LCD_ICON_PS_ZERO: converted_icon = LCD_SEG_PS_0; break;
        case LCD_ICON_PS_MINUS_1: converted_icon = LCD_SEG_PS_N1; break;
        case LCD_ICON_PS_MINUS_2: converted_icon = LCD_SEG_PS_N2; break;
        case LCD_ICON_PS_MINUS_3: converted_icon = LCD_SEG_PS_N3; break;
        case LCD_ICON_PS_MINUS_4: converted_icon = LCD_SEG_PS_N4; break;
        case LCD_ICON_PS_MINUS_5: converted_icon = LCD_SEG_PS_N5; break;
        case LCD_ICON_PS_MINUS_6: converted_icon = LCD_SEG_PS_N6; break;
        case LCD_ICON_PS_MINUS_7: converted_icon = LCD_SEG_PS_N7; break;
        case LCD_ICON_PS_MINUS_8: converted_icon = LCD_SEG_PS_N8; break;
        case LCD_ICON_PS_MINUS_9: converted_icon = LCD_SEG_PS_N9; break;
        case LCD_ICON_PS_MINUS_10: converted_icon = LCD_SEG_PS_N10; break;
        case LCD_ICON_PS_MINUS_11: converted_icon = LCD_SEG_PS_N11; break;
        case LCD_ICON_PS_MINUS_12: converted_icon = LCD_SEG_PS_N12; break;
        case LCD_ICON_PS_MINUS_13: converted_icon = LCD_SEG_PS_N13; break;
        case LCD_ICON_PS_MINUS_14: converted_icon = LCD_SEG_PS_N14; break;
        case LCD_ICON_PS_MINUS_15: converted_icon = LCD_SEG_PS_N15; break;
        case LCD_ICON_PS_MINUS_MINUS: converted_icon = LCD_SEG_PS_MM; break;
        default: return; // 未知图标类型
    }
    
    if(lcd_get_com_seg_from_icon(converted_icon, (uint8_t*)&com, (uint8_t*)&seg)) {
        lcd_set_ram_bit_internal(com, seg, enable);
    }
}

/**
  * @brief LCD清除图标
  * @param icon: 图标类型
  * @retval None
  */
void LCDDisplay_ClearIcon_New(LCD_Icon_TypeDef icon)
{
    LCDDisplay_ShowIcon_New(icon, 0);
}

/**
  * @brief 获取图标状态
  * @param icon: 图标类型
  * @retval 图标状态，1表示显示，0表示未显示
  */
uint8_t LCDDisplay_GetIconStatus_New(LCD_Icon_TypeDef icon)
{
    uint8_t com, seg;
    
    // 需要将 LCD_Icon_TypeDef 枚举转换为 lcd_segment_t 枚举
    lcd_segment_t converted_icon;
    switch(icon) {
        case LCD_ICON_MM: converted_icon = LCD_SEG_MM; break;
        case LCD_ICON_IN: converted_icon = LCD_SEG_IN; break;
        case LCD_ICON_REL: converted_icon = LCD_SEG_REL; break;
        case LCD_ICON_MAX: converted_icon = LCD_SEG_MAX; break;
        case LCD_ICON_MIN: converted_icon = LCD_SEG_MIN; break;
        case LCD_ICON_TIR: converted_icon = LCD_SEG_TIR; break;
        case LCD_ICON_TOL: converted_icon = LCD_SEG_TOL; break;
        case LCD_ICON_OK: converted_icon = LCD_SEG_OK; break;
        case LCD_ICON_NG: converted_icon = LCD_SEG_NG; break;
        case LCD_ICON_PS_PLUS_PLUS: converted_icon = LCD_SEG_PS_PP; break;
        case LCD_ICON_PS_PLUS_15: converted_icon = LCD_SEG_PS_P15; break;
        case LCD_ICON_PS_PLUS_14: converted_icon = LCD_SEG_PS_P14; break;
        case LCD_ICON_PS_PLUS_13: converted_icon = LCD_SEG_PS_P13; break;
        case LCD_ICON_PS_PLUS_12: converted_icon = LCD_SEG_PS_P12; break;
        case LCD_ICON_PS_PLUS_11: converted_icon = LCD_SEG_PS_P11; break;
        case LCD_ICON_PS_PLUS_10: converted_icon = LCD_SEG_PS_P10; break;
        case LCD_ICON_PS_PLUS_9: converted_icon = LCD_SEG_PS_P9; break;
        case LCD_ICON_PS_PLUS_8: converted_icon = LCD_SEG_PS_P8; break;
        case LCD_ICON_PS_PLUS_7: converted_icon = LCD_SEG_PS_P7; break;
        case LCD_ICON_PS_PLUS_6: converted_icon = LCD_SEG_PS_P6; break;
        case LCD_ICON_PS_PLUS_5: converted_icon = LCD_SEG_PS_P5; break;
        case LCD_ICON_PS_PLUS_4: converted_icon = LCD_SEG_PS_P4; break;
        case LCD_ICON_PS_PLUS_3: converted_icon = LCD_SEG_PS_P3; break;
        case LCD_ICON_PS_PLUS_2: converted_icon = LCD_SEG_PS_P2; break;
        case LCD_ICON_PS_PLUS_1: converted_icon = LCD_SEG_PS_P1; break;
        case LCD_ICON_PS_ZERO: converted_icon = LCD_SEG_PS_0; break;
        case LCD_ICON_PS_MINUS_1: converted_icon = LCD_SEG_PS_N1; break;
        case LCD_ICON_PS_MINUS_2: converted_icon = LCD_SEG_PS_N2; break;
        case LCD_ICON_PS_MINUS_3: converted_icon = LCD_SEG_PS_N3; break;
        case LCD_ICON_PS_MINUS_4: converted_icon = LCD_SEG_PS_N4; break;
        case LCD_ICON_PS_MINUS_5: converted_icon = LCD_SEG_PS_N5; break;
        case LCD_ICON_PS_MINUS_6: converted_icon = LCD_SEG_PS_N6; break;
        case LCD_ICON_PS_MINUS_7: converted_icon = LCD_SEG_PS_N7; break;
        case LCD_ICON_PS_MINUS_8: converted_icon = LCD_SEG_PS_N8; break;
        case LCD_ICON_PS_MINUS_9: converted_icon = LCD_SEG_PS_N9; break;
        case LCD_ICON_PS_MINUS_10: converted_icon = LCD_SEG_PS_N10; break;
        case LCD_ICON_PS_MINUS_11: converted_icon = LCD_SEG_PS_N11; break;
        case LCD_ICON_PS_MINUS_12: converted_icon = LCD_SEG_PS_N12; break;
        case LCD_ICON_PS_MINUS_13: converted_icon = LCD_SEG_PS_N13; break;
        case LCD_ICON_PS_MINUS_14: converted_icon = LCD_SEG_PS_N14; break;
        case LCD_ICON_PS_MINUS_15: converted_icon = LCD_SEG_PS_N15; break;
        case LCD_ICON_PS_MINUS_MINUS: converted_icon = LCD_SEG_PS_MM; break;
        default: return 0; // 未知图标类型
    }
    
    if(!(lcd_get_com_seg_from_icon(converted_icon, (uint8_t*)&com, (uint8_t*)&seg))) {
        return 0;
    }
    
    uint32_t mask = 1UL << seg;
    return (lcd_display_buffer_new[com] & mask) ? 1 : 0;
}

// 新增：根据单位和分辨率获取小数点位置
static uint8_t GetDecimalIconPosition(uint8_t is_metric, uint8_t res)
{
    if(is_metric)
    {
        switch(res)
        {
            case RESOLUTION_0_0005MM: return 2; // 0.5u分辨率：4位小数，小数点在第2位后（即第1位后）
            case RESOLUTION_0_001MM:  return 3; // 1u分辨率：3位小数，小数点在第3位后（即第2位后）
            case RESOLUTION_0_01MM:   return 4; // 0.01mm分辨率：2位小数，小数点在第4位后（即第3位后）
            default: return 3;
        }
    }
    else
    {
        // 英制单位的小数点位置需要向左移动一位
        switch(res)
        {
            case RESOLUTION_0_0005MM: return 1; // 0.5u分辨率：5位小数，小数点在第1位后（即第0位后）
            case RESOLUTION_0_001MM:  return 2; // 1u分辨率：4位小数，小数点在第2位后（即第1位后）
            case RESOLUTION_0_01MM:   return 3; // 0.01mm分辨率：3位小数，小数点在第3位后（即第2位后）
            default: return 2; // 默认向左移动一位
        }
    }
}
/**
  * @brief LCD显示特殊符号
  * @param symbol: 符号
  * @param pos: 位置
  * @retval None
  */
void LCDDisplay_ShowSymbol_New(const char* symbol, LCD_Position_TypeDef pos)
{
    if(symbol == NULL) return;
    
    if(symbol[0] >= '0' && symbol[0] <= '9') {
        LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(symbol[0] - '0');
        LCDDisplay_ShowChar_New(ch, pos);
    } else if(symbol[0] >= 'A' && symbol[0] <= 'Z') {
        LCD_Char_TypeDef ch = (LCD_Char_TypeDef)(symbol[0] - 'A' + LCD_CHAR_A);
        LCDDisplay_ShowChar_New(ch, pos);
    } else if(symbol[0] == '-') {
        LCDDisplay_ShowChar_New(LCD_CHAR_DASH, pos);
    } else {
        LCDDisplay_ShowChar_New(LCD_CHAR_BLANK, pos);
    }
}

/**
  * @brief LCD更新显示
  * @param None
  * @retval None
  */
void LCDDisplay_Update_New(void)
{
    // 将本地缓冲区内容有条件地写入LCD RAM寄存器（只更新变化的位）
    for(int com_index = 0; com_index < 4; com_index++)
    {
        int ram_index = com_index * 2;  // Map COM0->RAM0, COM1->RAM2, COM2->RAM4, COM3->RAM6
        
        uint32_t current_ram_value = hlcd.Instance->RAM[ram_index];
        uint32_t new_value = lcd_display_buffer_new[com_index];
        
        uint32_t changed_bits = current_ram_value ^ new_value;
        
        if(changed_bits != 0) {
            HAL_LCD_Write(&hlcd, ram_index, changed_bits, new_value);
        }
    }
    // 发起显示更新请求
    HAL_LCD_UpdateDisplayRequest(&hlcd);
}

/**
  * @brief 内部函数：字符到段码转换
  * @param ch: 字符
  * @retval 段码
  */
uint32_t LCDDisplay_CharToSegments_New(LCD_Char_TypeDef ch)
{
    if(ch <= LCD_CHAR_9) {
        return digit_patterns_new[ch - LCD_CHAR_0];
    } else if(ch == LCD_CHAR_NEGATIVE || ch == LCD_CHAR_DASH) {
        return 0x40; // g段
    } else if(ch == LCD_CHAR_BLANK) {
        return 0x00; // 不亮
    } else {
        return 0x00; // 默认不亮
    }
}

/**
  * @brief 设置指定位置的小数点
  * @param pos: 位置 (LCD_POSITION_1 到 LCD_POSITION_6)
  * @param enabled: 1为启用小数点，0为禁用
  * @retval None
  */
static void set_decimal_point_for_position(LCD_Position_TypeDef pos, uint8_t enabled)
{
    if(pos <= LCD_POSITION_6) {
        decimal_point_positions[pos] = enabled;
        
        // 根据位置设置对应的小数点段
        switch(pos) {
            case LCD_POSITION_1:
                lcd_set_ram_bit_internal(3, 2, enabled); // COM4/SEG2
                break;
            case LCD_POSITION_2:
                lcd_set_ram_bit_internal(3, 4, enabled); // COM4/SEG4
                break;
            case LCD_POSITION_3:
                lcd_set_ram_bit_internal(3, 6, enabled); // COM4/SEG6
                break;
            case LCD_POSITION_4:
                lcd_set_ram_bit_internal(3, 8, enabled); // COM4/SEG8
                break;
            case LCD_POSITION_5:
                lcd_set_ram_bit_internal(3, 10, enabled); // COM4/SEG10
                break;
            case LCD_POSITION_6:
                lcd_set_ram_bit_internal(3, 12, enabled); // COM4/SEG12
                break;
            default:
                break;
        }
        
        // 同时更新本地缓冲区
        switch(pos) {
            case LCD_POSITION_1:
                enabled ? (lcd_display_buffer_new[3] |= (1UL << 2)) : (lcd_display_buffer_new[3] &= ~(1UL << 2));
                break;
            case LCD_POSITION_2:
                enabled ? (lcd_display_buffer_new[3] |= (1UL << 4)) : (lcd_display_buffer_new[3] &= ~(1UL << 4));
                break;
            case LCD_POSITION_3:
                enabled ? (lcd_display_buffer_new[3] |= (1UL << 6)) : (lcd_display_buffer_new[3] &= ~(1UL << 6));
                break;
            case LCD_POSITION_4:
                enabled ? (lcd_display_buffer_new[3] |= (1UL << 8)) : (lcd_display_buffer_new[3] &= ~(1UL << 8));
                break;
            case LCD_POSITION_5:
                enabled ? (lcd_display_buffer_new[3] |= (1UL << 10)) : (lcd_display_buffer_new[3] &= ~(1UL << 10));
                break;
            case LCD_POSITION_6:
                enabled ? (lcd_display_buffer_new[3] |= (1UL << 12)) : (lcd_display_buffer_new[3] &= ~(1UL << 12));
                break;
        }
    }
}

/**
  * @brief 内部函数：设置或清除LCD RAM中的特定位
  * @param com: COM线 (0-3)
  * @param seg: SEG线 (0-25)
  * @param state: 1=设置，0=清除
  * @retval None
  */
static void lcd_set_ram_bit_internal(uint8_t com, uint8_t seg, uint8_t state)
{
    if(com > 3) return;
    
    if(state) {
        lcd_display_buffer_new[com] |= (1UL << seg);
    } else {
        lcd_display_buffer_new[com] &= ~(1UL << seg);
    }
}

/**
 * @brief 内部辅助函数 - 显示负号（始终使用SIGN图标）
 * @param is_negative: 是否为负数
 * @retval None
 */
static void lcd_show_negative_sign(uint8_t is_negative)
{
    if (!is_negative) {
        // 清除负号显示
        lcd_set_ram_bit_internal(2, 0, 0); // SIGN图标对应LCD_SEG_SIGN
        return;
    }
    
    // 总是使用SIGN图标显示负号
    lcd_set_ram_bit_internal(2, 0, 1); // SIGN图标
}

/**
  * @brief 内部辅助函数 - 显示单位图标（mm/in）
  * @param is_metric: 是否公制（mm=1，in=0）
  * @retval None
  */
static void lcd_show_unit_icon(uint8_t is_metric)
{
    if(is_metric) {
        LCDDisplay_ShowIcon_New(LCD_ICON_MM, 1);
        LCDDisplay_ClearIcon_New(LCD_ICON_IN);
    } else {
        LCDDisplay_ShowIcon_New(LCD_ICON_IN, 1);
        LCDDisplay_ClearIcon_New(LCD_ICON_MM);
    }
}

/**
  * @brief 内部辅助函数 - 显示模式图标（REL/MAX/MIN/TIR/TOL）
  * @param mode: 当前测量模式
  * @retval None
  */
static void lcd_show_mode_icon(MeasurementMode_TypeDef mode)
{
    // 先清除所有模式图标
    LCDDisplay_ClearIcon_New(LCD_ICON_REL);
    LCDDisplay_ClearIcon_New(LCD_ICON_MAX);
    LCDDisplay_ClearIcon_New(LCD_ICON_MIN);
    LCDDisplay_ClearIcon_New(LCD_ICON_TIR);
    LCDDisplay_ClearIcon_New(LCD_ICON_TOL);
    
    // 显示当前模式图标
    switch(mode) {
        case MEASUREMENT_MODE_REL:
            LCDDisplay_ShowIcon_New(LCD_ICON_REL, 1);
            break;
        case MEASUREMENT_MODE_MAX:
            LCDDisplay_ShowIcon_New(LCD_ICON_MAX, 1);
            break;
        case MEASUREMENT_MODE_MIN:
            LCDDisplay_ShowIcon_New(LCD_ICON_MIN, 1);
            break;
        case MEASUREMENT_MODE_TIR:
            LCDDisplay_ShowIcon_New(LCD_ICON_TIR, 1);
            break;
        case MEASUREMENT_MODE_TOL_SETTING:
            // TOL设置模式：显示TOL图标
            LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);
            break;
        case MEASUREMENT_MODE_NORMAL:
            // NORMAL模式：不显示REL图标，也不显示其他模式图标（MAX/MIN/TIR/TOL）
            // 如果系统处于相对测量模式，但当前是NORMAL子模式，我们可能需要特殊的处理
            // 对于相对测量的NORMAL模式，仍然显示REL图标以表明处于相对测量模式
            // 实际上，如果用户处于相对测量模式，应该在NORMAL模式下也显示REL图标
            // 但为了区分NORMAL子模式和其他子模式，我们不额外显示任何子模式图标
            break;
        default:
            break;
    }
}

/**
  * @brief 内部辅助函数 - 显示公差图标（OK/NG）
  * @param display_value: 当前显示值
  * @retval None
  */
static void lcd_show_tolerance_icon(int32_t display_value)
{
    if(measurement_state.is_tolerance_enabled) {
        ToleranceResult_TypeDef tol_result = ZTJ_GetToleranceResult(display_value);
        if(tol_result == TOLERANCE_RESULT_OK) {
            LCDDisplay_ShowIcon_New(LCD_ICON_OK, 1);
            LCDDisplay_ClearIcon_New(LCD_ICON_NG);
					 LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);
        } else {
            LCDDisplay_ClearIcon_New(LCD_ICON_OK);
            LCDDisplay_ShowIcon_New(LCD_ICON_NG, 1);
					 LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);
        }
    } else {
        LCDDisplay_ClearIcon_New(LCD_ICON_OK);
        LCDDisplay_ClearIcon_New(LCD_ICON_NG);
    }
}

/**
  * @brief 新增：统一显示接口 - 输入处理后的数据结构体，自动完成所有显示
  * @param processed_data: 处理后的传感器数据结构体
  * @retval None
  */
 void LCDDisplay_ShowProcessedData(SensorData_TypeDef processed_data)
 {
     // 1. 清屏初始化
     LCDDisplay_Clear_New();
     
     // 2. 核心参数获取
     uint8_t res = measurement_state.current_resolution;
     // 从LCD显示状态机获取单位设置，而不是直接从测量状态机获取
     LCD_DisplayState_TypeDef* lcd_display_state_ptr = LCD_DisplayState_GetCurrent();
     uint8_t is_metric = lcd_display_state_ptr->is_metric; // 使用is_metric字段而不是unit字段
     
     
     // 添加单位同步检查，确保LCD显示状态机与测量状态机同步
     extern ZTJ_TypeDef measurement_state;
     uint8_t expected_is_metric = (measurement_state.unit == 0) ? 1 : 0;
     if (is_metric != expected_is_metric) {
         // 同步不一致，更新LCD显示状态机
         lcd_display_state_ptr->is_metric = expected_is_metric;
         is_metric = expected_is_metric;
     }
     uint8_t is_negative = processed_data.sign;               // sign=1→负，0→正
     uint8_t is_6digit = (processed_data.value >= 100000) ? 1 : 0; // value是绝对值，直接判断
     
     
     int32_t display_value = processed_data.value;            // 赋值处理后的绝对值
     // 数字 already in processed_data.digits
     // 使用processed_data自带的digits数组，而不是重新计算
     
     // 从LCD显示状态机获取当前显示设置，而不是直接使用processed_data中的单位
     // 注意：lcd_display_state已在上面声明，无需重复声明
     // uint8_t is_metric = (lcd_display_state->unit == 0) ? 1 : 0; // 从显示状态机获取单位设置（已在上面定义）
     
     
     // 3. 显示负号
     lcd_show_negative_sign(is_negative);
     
     // 4. 显示数字和小数点（按分辨率适配）
     if (res == RESOLUTION_0_0005MM) { // 0.5μm模式：使用统一的显示逻辑，不再根据数值大小动态调整小数点
         // 与其它分辨率一样的处理逻辑，确保小数点位置一致
         uint8_t should_display_as_6digits = (processed_data.value >= 100000) ? 1 : 0;
         
         if (should_display_as_6digits) {
             // 显示全部6位数字，注意digits数组索引与显示位置的对应关系
             // digits[0]是最高位（十万位），digits[5]是最低位（个位）
             
             
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[0], (LCD_Position_TypeDef)LCD_POSITION_1); // 最高位（十万位）
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[1], (LCD_Position_TypeDef)LCD_POSITION_2); // 次高位（万位）
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[2], (LCD_Position_TypeDef)LCD_POSITION_3); // 千位
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[3], (LCD_Position_TypeDef)LCD_POSITION_4); // 百位
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[4], (LCD_Position_TypeDef)LCD_POSITION_5); // 十位
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[5], (LCD_Position_TypeDef)LCD_POSITION_6); // 最低位（个位）
         } else {
             // 对于非6位数字，实现前导零消除功能，但保留小数点前的零
             // 确定第一个非零数字的位置（处理前导零）
             uint8_t first_nonzero_pos = 0;
             bool all_zeros = true;  // 添加标志来检查是否所有数字都是0

             for(int i = 0; i < 6; i++) {
                 if(processed_data.digits[i] != 0) { // 找到第一个非零数字
                     first_nonzero_pos = i;
                     all_zeros = false;
                     break;
                 }
             }

             // 获取小数点位置，确保小数点前的零不会被消除
             uint8_t decimal_pos = GetDecimalIconPosition(is_metric, res);
             if (decimal_pos >= 1 && decimal_pos <= 5) {
                 decimal_pos--; // 转换为数组索引（0-5）
             } else {
                 decimal_pos = 6; // 如果没有小数点或小数点在最后，则设为最大值
             }

             // 修正逻辑：全零值按分辨率显示，根据小数点位置消隐前导零
             // 规则：小数点在第二位则消隐第一位零，小数点在第三位则消隐第一、二位，类推
             if(all_zeros) {
                 // 全零值：根据小数点位置决定哪些位置显示0，哪些位置显示空白
                 for(int i = 0; i < 6; i++) {
                     if(i >= decimal_pos) {
                         // 小数点位置及之后显示0
                         LCDDisplay_ShowChar_New(LCD_CHAR_0, (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     } else {
                         // 小数点前的位置显示空白（实现前导零消隐）
                         LCDDisplay_ShowChar_New(LCD_CHAR_BLANK, (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     }
                 }
             } else {
                 // 非全零值：应用前导零消除逻辑
                 for(int i = 0; i < 6; i++) {
                     // 首先确定这个位置是否在小数点前
                     bool is_before_decimal = (i < decimal_pos);
                     
                     if(is_before_decimal && i < first_nonzero_pos) {
                         // 小数点之前的前导零位置显示空白
                         LCDDisplay_ShowChar_New(LCD_CHAR_BLANK, (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     } else {
                         // 其他位置正常显示数字
                         LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[i], (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     }
                 }
             }
         }
         
         // 小数点：按分辨率/单位计算位置（在同一分辨率下位置固定）
         uint8_t decimal_pos = GetDecimalIconPosition(is_metric, res);
         if (decimal_pos >= 1 && decimal_pos <= 5) {
             // 小数点位置固定，不受6位/5位显示模式影响
             set_decimal_point_for_position((uint8_t)(decimal_pos - 1), 1);
         }
     } else { // 1μm/0.01mm模式 - 改进6位/非6位显示逻辑
         // 重新定义6位显示逻辑：根据数值大小和显示需求判断
         uint8_t should_display_as_6digits = (processed_data.value >= 100000) ? 1 : 0;
         
         if (should_display_as_6digits) {
             // 显示全部6位数字，注意digits数组索引与显示位置的对应关系
             // digits[0]是最高位（十万位），digits[5]是最低位（个位）
             
             
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[0], (LCD_Position_TypeDef)LCD_POSITION_1); // 最高位（十万位）
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[1], (LCD_Position_TypeDef)LCD_POSITION_2); // 次高位（万位）
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[2], (LCD_Position_TypeDef)LCD_POSITION_3); // 千位
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[3], (LCD_Position_TypeDef)LCD_POSITION_4); // 百位
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[4], (LCD_Position_TypeDef)LCD_POSITION_5); // 十位
             LCDDisplay_ShowChar_New((LCD_Char_TypeDef)processed_data.digits[5], (LCD_Position_TypeDef)LCD_POSITION_6); // 最低位（个位）
         } else {
             // 对于非6位数字，实现前导零消除功能，但保留小数点前的零
             // 确定第一个非零数字的位置（处理前导零）
             uint8_t first_nonzero_pos = 0;
             bool all_zeros = true;  // 添加标志来检查是否所有数字都是0

             for(int i = 0; i < 6; i++) {
                 if(processed_data.digits[i] != 0) { // 找到第一个非零数字
                     first_nonzero_pos = i;
                     all_zeros = false;
                     break;
                 }
             }

             // 获取小数点位置，确保小数点前的零不会被消除
             uint8_t decimal_pos = GetDecimalIconPosition(is_metric, res);
             if (decimal_pos >= 1 && decimal_pos <= 5) {
                 decimal_pos--; // 转换为数组索引（0-5）
             } else {
                 decimal_pos = 6; // 如果没有小数点或小数点在最后，则设为最大值
             }

             // 修正逻辑：全零值按分辨率显示，根据小数点位置消隐前导零
             // 规则：小数点在第二位则消隐第一位零，小数点在第三位则消隐第一、二位，类推
             if(all_zeros) {
                 // 全零值：根据小数点位置决定哪些位置显示0，哪些位置显示空白
                 for(int i = 0; i < 6; i++) {
                     if(i >= decimal_pos) {
                         // 小数点位置及之后显示0
                         LCDDisplay_ShowChar_New(LCD_CHAR_0, (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     } else {
                         // 小数点前的位置显示空白（实现前导零消隐）
                         LCDDisplay_ShowChar_New(LCD_CHAR_BLANK, (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     }
                 }
             } else {
                 // 非全零值：应用前导零消除逻辑
                 for(int i = 0; i < 6; i++) {
                     // 首先确定这个位置是否在小数点前
                     bool is_before_decimal = (i < decimal_pos);
                     
                     if(is_before_decimal && i < first_nonzero_pos) {
                         // 小数点之前的前导零位置显示空白
                         LCDDisplay_ShowChar_New(LCD_CHAR_BLANK, (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     } else {
                         // 其他位置正常显示数字
                         LCDDisplay_ShowChar_New(processed_data.digits[i], (LCD_Position_TypeDef)(LCD_POSITION_1 + i));
                     }
                 }
             }
         }
         
         // 小数点：按分辨率/单位计算位置（在同一分辨率下位置固定）
         uint8_t decimal_pos = GetDecimalIconPosition(is_metric, res);
         if (decimal_pos >= 1 && decimal_pos <= 5) {
             // 小数点位置固定，不受6位/5位显示模式影响
             set_decimal_point_for_position((uint8_t)(decimal_pos - 1), 1);
         }
     }
     
     // 5. 显示单位图标
     lcd_show_unit_icon(is_metric);
     
     
     // 6. 显示模式图标
     lcd_show_mode_icon((MeasurementMode_TypeDef)ZTJ_GetCurrentMode()); // 调用状态机接口获取当前模式，替代直接访问成员
     
     // 7. 显示公差图标（在公差设置模式和公差检查状态下显示OK/NG状态）
     // 首先处理TOL图标显示
     if(measurement_state.is_tolerance_enabled && !measurement_state.is_in_tolerance_check) {
         // 在启用了公差功能但未进入公差检查状态时显示TOL图标
         // 这适用于NORMAL、ABS、REL模式下启用了公差检查的情况
         LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);
     } else {
         LCDDisplay_ClearIcon_New(LCD_ICON_TOL);
     }
     
     if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
         // 在公差设置模式下，不显示OK/NG图标，而是显示当前设置状态
         // 可以根据需要在这里添加公差设置指示图标
         LCDDisplay_ClearIcon_New(LCD_ICON_OK);
         LCDDisplay_ClearIcon_New(LCD_ICON_NG);
     } else {
         // 非公差设置模式下按原逻辑处理
         // 但如果当前处于公差检查状态，则显示公差判断结果
         if(measurement_state.is_in_tolerance_check) {
             // 在公差检查状态下，根据当前值与公差范围的比较结果显示OK/NG图标
             ToleranceResult_TypeDef tol_result = ZTJ_GetToleranceResult(display_value);
             if(tol_result == TOLERANCE_RESULT_OK) {
                 LCDDisplay_ShowIcon_New(LCD_ICON_OK, 1);   // 显示OK图标表示在公差范围内
                 LCDDisplay_ClearIcon_New(LCD_ICON_NG);   // 确保NG图标关闭
             } else {
                 LCDDisplay_ClearIcon_New(LCD_ICON_OK);   // 确保OK图标关闭
                 LCDDisplay_ShowIcon_New(LCD_ICON_NG, 1);   // 显示NG图标表示超出公差范围
             }
         } else {
             // 非公差检查状态下按原逻辑处理
             lcd_show_tolerance_icon(display_value);
         }
     }
     
     // 8. 特殊处理：公差设置模式下的显示
     if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
         // 在公差设置模式下，根据tolerance_setting_step显示当前设置的公差值和TOL+MAX或TOL+MIN
         // tolerance_setting_step: 0=设置上公差(正公差)，1=设置下公差(负公差)
         
         // 清除当前显示，准备显示公差值
         LCDDisplay_Clear_New();
         
         if(measurement_state.tolerance_setting_step == 0) {
             // 正在设置上公差（正公差），显示当前上公差值和TOL+MAX图标
             LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);
             LCDDisplay_ShowIcon_New(LCD_ICON_MAX, 1);
             
             // 显示当前上公差值
             int32_t tolerance_value = measurement_state.tolerance_upper;
             
             // 由于公差值可能是较大数字，需要将其转换为6位数字显示
             uint8_t digits[6] = {0};
             int32_t temp_val = tolerance_value < 0 ? -tolerance_value : tolerance_value; // 使用绝对值进行分解
             
             // 分解数字到各个位
             for(uint8_t i = 0; i < 6; i++) {
                 digits[5 - i] = temp_val % 10;
                 temp_val /= 10;
             }
             
             // 显示数字（从高位到低位）
             for(uint8_t pos = 0; pos < 6; pos++) {
                 LCDDisplay_ShowChar_New((LCD_Char_TypeDef)digits[pos], (LCD_Position_TypeDef)pos);
             }
             
             // 如果是负数，显示负号图标
             if(tolerance_value < 0) {
                 lcd_show_negative_sign(1); // 显示负号
             } else {
                 lcd_show_negative_sign(0); // 不显示负号
             }
             
             // 根据分辨率和单位设置小数点位置
             uint8_t is_metric = (measurement_state.unit == 0) ? 1 : 0;
             uint8_t res = measurement_state.current_resolution;
             uint8_t decimal_pos = GetDecimalIconPosition(is_metric, res);
             if(decimal_pos >= 1 && decimal_pos <= 5) {
                 set_decimal_point_for_position((uint8_t)(decimal_pos - 1), 1);
             }
         } else if(measurement_state.tolerance_setting_step == 1) {
             // 正在设置下公差（负公差），显示当前下公差值和TOL+MIN图标
             LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);
             LCDDisplay_ShowIcon_New(LCD_ICON_MIN, 1);
             
             // 显示当前下公差值
             int32_t tolerance_value = measurement_state.tolerance_lower;
             
             // 由于公差值可能是较大数字，需要将其转换为6位数字显示
             uint8_t digits[6] = {0};
             int32_t temp_val = tolerance_value < 0 ? -tolerance_value : tolerance_value; // 使用绝对值进行分解
             
             // 分解数字到各个位
             for(uint8_t i = 0; i < 6; i++) {
                 digits[5 - i] = temp_val % 10;
                 temp_val /= 10;
             }
             
             // 显示数字（从高位到低位）
             for(uint8_t pos = 0; pos < 6; pos++) {
                 LCDDisplay_ShowChar_New((LCD_Char_TypeDef)digits[pos], (LCD_Position_TypeDef)pos);
             }
             
             // 如果是负数，显示负号图标
             if(tolerance_value < 0) {
                 lcd_show_negative_sign(1); // 显示负号
             } else {
                 lcd_show_negative_sign(0); // 不显示负号
             }
             
             // 根据分辨率和单位设置小数点位置
             uint8_t is_metric = (measurement_state.unit == 0) ? 1 : 0;
             uint8_t res = measurement_state.current_resolution;
             uint8_t decimal_pos = GetDecimalIconPosition(is_metric, res);
             if(decimal_pos >= 1 && decimal_pos <= 5) {
                 set_decimal_point_for_position((uint8_t)(decimal_pos - 1), 1);
             }
         }
     }
     
     // 9. 特殊处理：模拟指针显示
     // PS0（指针零位）应在进入相对测量模式时一直显示
     // MAX/MIN/TIR模式只在相对测量时有效（REL/NORMAL模式下）
     MeasurementMode_TypeDef current_mode = (MeasurementMode_TypeDef)ZTJ_GetCurrentMode();
     if(current_mode == MEASUREMENT_MODE_REL || current_mode == MEASUREMENT_MODE_NORMAL ||
        current_mode == MEASUREMENT_MODE_MAX || current_mode == MEASUREMENT_MODE_MIN ||
        current_mode == MEASUREMENT_MODE_TIR || current_mode == MEASUREMENT_MODE_TOL_SETTING) {
        // 在相对测量模式（REL/MAX/MIN/TIR/TOL_SETTING）下显示指针
        
        // PS0图标在指针模拟模式下始终显示
        LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // PS0图标常亮
        
        // 特殊处理TIR模式：显示最大正跳动和最大负跳动
        if(current_mode == MEASUREMENT_MODE_TIR) {
            // 获取TIR模式下的最大正跳动和最大负跳动值
            int32_t max_positive_dev = ZTJ_GetTirMaxPositiveDeviation();
            int32_t max_negative_dev = ZTJ_GetTirMaxNegativeDeviation();
            
            // 计算模拟指针值，基于当前偏差相对于最大偏差的比例
            int32_t current_deviation = processed_data.value - ZTJ_GetTirBaseValue();
            
            // 清除所有PS+和PS-图标
            for(int i = 1; i <= 15; i++) {
                 LCDDisplay_ClearIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_PLUS_1 + i - 1));// 清除PS+1到PS+15图标
                 LCDDisplay_ClearIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_MINUS_1 + i - 1));// 清除PS-1到PS-15图标
							LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // PS0图标常亮
            }
						   
            // 根据当前偏差点亮相应的PS+或PS-图标
            if(current_deviation > 0) {
                // 计算相对于最大正跳动的比例，然后映射到1-15的图标
                // 修复除零错误：当max_positive_dev为0时，使用当前偏差的绝对值作为参考
                float reference_value = (max_positive_dev == 0) ? (float)current_deviation : (float)max_positive_dev;
                float ratio = (float)current_deviation / reference_value;
                
                // 限制比例在合理范围内
                if(ratio > 1.0f) ratio = 1.0f;
                if(ratio < 0.0f) ratio = 0.0f;
                
                int ps_level = (int)(ratio * 15);
                if(ps_level > 15) ps_level = 15;
                if(ps_level < 1) ps_level = 1;
                
                 // 点亮PS+图标，点亮从1到ps_level的所有图标（柱状显示）
                for(int i = 1; i <= ps_level; i++) {
                     LCDDisplay_ShowIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_PLUS_1 + i - 1), 1);
                     LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1);
                }
                
                // 更新模拟指针值为正值
                analog_pointer_value = ps_level;
            } else if(current_deviation < 0) {
                // 计算相对于最大负跳动的比例，然后映射到1-15的图标
                // 修复除零错误：当max_negative_dev为0时，使用当前偏差的绝对值作为参考
                int32_t abs_max_negative_dev = (max_negative_dev == 0) ? (-current_deviation) : (-max_negative_dev);
                float ratio = (float)(-current_deviation) / (float)abs_max_negative_dev;
                
                // 限制比例在合理范围内
                if(ratio > 1.0f) ratio = 1.0f;
                if(ratio < 0.0f) ratio = 0.0f;
                
                int ps_level = (int)(ratio * 15);
                if(ps_level > 15) ps_level = 15;
                if(ps_level < 1) ps_level = 1;
                
                 // 点亮PS-图标，点亮从1到ps_level的所有图标（柱状显示）
                for(int i = 1; i <= ps_level; i++) {
                     LCDDisplay_ShowIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_MINUS_1 + i - 1), 1);
                     LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1);
                }
                
                // 更新模拟指针值为负值
                analog_pointer_value = -ps_level;
            } else {
                // 当前偏差为0，只点亮PS0，指针值为0
                analog_pointer_value = 0;
                 LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1);
            }
            
            // PS++和PS--超限指示仍然需要单独处理
            if(max_positive_dev > 15) {
                LCDDisplay_ShowIcon_New(LCD_ICON_PS_PLUS_PLUS, 1);   // 显示PS++图标 (最大正跳动超限)
            } else {
                LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_PLUS);     // 清除PS++图标
            }
            
            if(max_negative_dev < -15) {
                LCDDisplay_ShowIcon_New(LCD_ICON_PS_MINUS_MINUS, 1);   // 显示PS--图标 (最大负跳动超限)
            } else {
                LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_MINUS);     // 清除PS--图标
            }
        } else {
            // 非TIR模式下，按原来的方式显示模拟指针
            // PS++和PS--超限指示仍然需要单独处理
            if(analog_pointer_value > 15) {
                LCDDisplay_ShowIcon_New(LCD_ICON_PS_PLUS_PLUS, 1);   // 显示PS++图标 (ps++)
            } else {
                LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_PLUS);     // 清除PS++图标
            }
            
            if(analog_pointer_value < -15) {
                LCDDisplay_ShowIcon_New(LCD_ICON_PS_MINUS_MINUS, 1);   // 显示PS--图标 (ps--)
            } else {
                LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_MINUS);     // 清除PS--图标
            }

            // 清除所有PS+图标
             for(int i = 1; i <= 15; i++) {
                 LCDDisplay_ClearIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_PLUS_1 + i-1));//LCD_ICON_PS_MINUS_1 + i - 1
							 LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // PS0图标常亮
             }

            // 清除所有PS-图标
             for(int i = 1; i <= 15; i++) {
                 LCDDisplay_ClearIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_MINUS_1 + i-1 ));
							 LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // PS0图标常亮
             }

             // 根据模拟指针值点亮相应的PS+或PS-图标
             if(analog_pointer_value > 0) {
                 // 限制指针值在有效范围内
                 int abs_analog_value = (analog_pointer_value > 15) ? 15 : analog_pointer_value;
            if(abs_analog_value > 0) {
                     // 点亮PS+图标，点亮从1到abs_analog_value的所有图标
									   LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // 确保PS0图标始终显示
                     for(int i = 1; i <= abs_analog_value; i++) {
                        LCDDisplay_ShowIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_PLUS_1 + i - 1), 1);
                    }
                 }
             } else if(analog_pointer_value < 0) {
                 // 限制指针值在有效范围内
                 int abs_analog_value = (-analog_pointer_value > 15) ? 15 : (-analog_pointer_value);
							 LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // 确保PS0图标始终显示
                 if(abs_analog_value > 0) {
                     // 点亮PS-图标，点亮从1到abs_analog_value的所有图标
                     for(int i = 1; i <= abs_analog_value; i++) {
                        LCDDisplay_ShowIcon_New((LCD_Icon_TypeDef)(LCD_ICON_PS_MINUS_1 + i - 1), 1);
                    }
                }
            }
        }
    } else {
        // 在绝对测量模式下，不显示指针或显示特定内容
        // PS0图标不能清除，因为PS0和标尺是同一个，必须始终显示
	 
        LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // 确保PS0图标始终显示
        // 清除PS++图标
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_PLUS); // 清除PS++图标
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_MINUS); // 清除PS--图标
        // 清除所有PS+图标
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_1);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_2);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_3);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_4);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_5);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_6);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_7);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_8);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_9);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_10);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_11);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_12);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_13);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_14);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_PLUS_15);
        // 清除所有PS-图标
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_1);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_2);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_3);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_4);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_5);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_6);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_7);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_8);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_9);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_10);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_11);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_12);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_13);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_14);
        LCDDisplay_ClearIcon_New(LCD_ICON_PS_MINUS_15);
    }
        LCDDisplay_ShowIcon_New(LCD_ICON_PS_ZERO, 1); // PS0图标常亮
    // 9. 根据按键状态机更新LCD显示图标（已由按键处理函数直接处理）
    // 现在按键相关的显示更新由按键处理函数直接处理，此处不再依赖key_fsm状态
     
    // 10. 发起显示更新，将缓冲区内容刷新到LCD硬件
    LCDDisplay_Update_New();
     
    // 添加额外的调试信息，如果启用了LCD显示调试
    #ifdef DEBUG_LCD_DISPLAY
    // 可以添加一些状态指示，但避免过多影响性能
    #endif
 }

/* USER CODE END 0 */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/**
 * @brief LCD显示状态机初始化函数
 * @param None
 * @retval None
 */
void LCD_DisplayState_Init(void)
{
    // 初始化LCD显示状态机的默认值
    // 从测量状态机获取初始单位设置，确保同步
    extern ZTJ_TypeDef measurement_state;
    extern LCD_DisplayState_TypeDef lcd_display_state;
    lcd_display_state.is_metric = (measurement_state.unit == 0) ? 1 : 0;  // 根据测量状态机的单位设置
    lcd_display_state.mode = 0;  // 默认模式
    lcd_display_state.resolution = 0;  // 默认分辨率
    lcd_display_state.decimal_position = 0;  // 默认小数点位置
    
    // 初始化所有图标状态为关闭
    for(int i = 0; i < LCD_ICON_MAX_NUM; i++) {
        lcd_display_state.icons_status[i] = 0;
    }
}

/**
 * @brief 设置LCD显示单位
 * @param is_metric: 是否公制 (1=MM, 0=IN)
 * @retval None
 */
void LCD_DisplayState_SetMetric(uint8_t is_metric)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    lcd_display_state.is_metric = is_metric;
}

/**
 * @brief 设置LCD显示单位（通过unit值转换为is_metric）
 * @param unit: 单位值 (0=MM, 1=IN)
 * @retval None
 */
void LCD_DisplayState_SetUnit(uint8_t unit)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    // unit=0(MM)→is_metric=1，unit=1(IN)→is_metric=0
    lcd_display_state.is_metric = (unit == 0) ? 1 : 0;
    // 添加调试输出，如果启用了调试功能
    #ifdef DEBUG_LCD_DISPLAY
    char debug_str[16];
    sprintf(debug_str, "UN:%d,%d", unit, lcd_display_state.is_metric);
    LCDDisplay_ShowString_New(debug_str);
    LCDDisplay_Update_New();
    HAL_Delay(500);
    #endif
}

/**
 * @brief 设置LCD显示模式
 * @param mode: 模式值
 * @retval None
 */
void LCD_DisplayState_SetMode(uint8_t mode)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    lcd_display_state.mode = mode;
}

/**
 * @brief 设置LCD显示分辨率
 * @param resolution: 分辨率值
 * @retval None
 */
void LCD_DisplayState_SetResolution(uint8_t resolution)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    lcd_display_state.resolution = resolution;
}

/**
 * @brief 设置LCD小数点位置
 * @param position: 小数点位置
 * @retval None
 */
void LCD_DisplayState_SetDecimalPosition(uint8_t position)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    lcd_display_state.decimal_position = position;
}

/**
 * @brief 设置LCD图标状态
 * @param icon: 图标类型
 * @param enable: 1=显示，0=隐藏
 * @retval None
 */
void LCD_DisplayState_SetIcon(LCD_Icon_TypeDef icon, uint8_t enable)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    if(icon < LCD_ICON_MAX_NUM) {
        lcd_display_state.icons_status[icon] = enable;
    }
}

/**
 * @brief 获取LCD图标状态
 * @param icon: 图标类型
 * @retval 1=显示，0=隐藏
 */
uint8_t LCD_DisplayState_GetIconStatus(LCD_Icon_TypeDef icon)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    if(icon < LCD_ICON_MAX_NUM) {
        return lcd_display_state.icons_status[icon];
    }
    return 0;
}

/**
 * @brief 获取当前LCD显示状态机指针
 * @param None
 * @retval LCD显示状态机指针
 */
LCD_DisplayState_TypeDef* LCD_DisplayState_GetCurrent(void)
{
    extern LCD_DisplayState_TypeDef lcd_display_state;
    return &lcd_display_state;
}

/* USER CODE END 1 */

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(__ICCARM__)
#pragma diag_default=Pe177
#elif defined(__CC_ARM)
#pragma diag_default=177
#endif
