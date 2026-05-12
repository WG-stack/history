/**
  ******************************************************************************
  * @file           : lcd_display.c
  * @brief          : 段码屏LCD显示模块实现
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lcd_display.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MAX_DIGITS  6  // 6位数字显示

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// 根据用户提供的映射表：
// 数字1: SEG1(F,G,E,D at COM0,1,2,3), SEG2(A,B,C at COM0,1,2)
// 数字2: SEG3(F,G,E,D at COM0,1,2,3), SEG4(A,B,C at COM0,1,2)
// ...
// 每个数字占用2个SEG，分别显示不同的段

// 数字起始SEG位置 [数字1, 数字2, 数字3, 数字4, 数字5, 数字6]
static const uint8_t DISP_NUM[6] = {1, 3, 5, 7, 9, 11};

// 数字0-9的段码表 [数字][COM行]
// 每行的值 = F*1 + A*2 (COM0), G*1 + B*2 (COM1), E*1 + C*2 (COM2), D*1 (COM3)
// 7段显示：a(上), b(右上), c(右下), d(下), e(左下), f(左上), g(中)
// 注意：COM3只有D段，权值为1，不是2
static const uint8_t DISP_NUM_TAB[10][4] = {
    {3, 2, 3, 1},  // 0: F=1,A=1, G=0,B=1, E=1,C=1, D=1 -> abcdef
    {0, 2, 2, 0},  // 1: F=0,A=0, G=0,B=1, E=0,C=1, D=0 -> bc
    {2, 3, 1, 1},  // 2: F=0,A=1, G=1,B=1, E=1,C=0, D=1 -> abdeg
    {2, 3, 2, 1},  // 3: F=0,A=1, G=1,B=1, E=0,C=1, D=1 -> abcdg (修正COM1: 0+2=2 改为 1+2=3)
    {1, 3, 2, 0},  // 4: F=1,A=0, G=1,B=1, E=0,C=1, D=0 -> bcfg (修正COM1: 0+2=2 改为 1+2=3)
    {3, 1, 2, 1},  // 5: F=1,A=1, G=1,B=0, E=0,C=1, D=1 -> acdfg (修正COM1: 0+0=0 改为 1+0=1)
    {3, 1, 3, 1},  // 6: F=1,A=1, G=1,B=0, E=1,C=1, D=1 -> acdefg
    {2, 2, 2, 0},  // 7: F=0,A=1, G=0,B=1, E=0,C=1, D=0 -> abc
    {3, 3, 3, 1},  // 8: F=1,A=1, G=1,B=1, E=1,C=1, D=1 -> abcdefg
    {3, 3, 2, 1}   // 9: F=1,A=1, G=1,B=1, E=0,C=1, D=1 -> abcdfg (修正COM1: 0+2=2 改为 1+2=3)
};

// COM到RAM寄存器的映射
static const uint32_t COM_TO_RAM[4] = {
    LCD_RAM_REGISTER0,  // COM0
    LCD_RAM_REGISTER2,  // COM1
    LCD_RAM_REGISTER4,  // COM2
    LCD_RAM_REGISTER6   // COM3
};

/* Private function prototypes -----------------------------------------------*/
static void Convert_NumberToDigits(float number, uint8_t decimal_places, uint8_t *digits, uint8_t *digit_count);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化LCD显示模块
  * @retval None
  */
void LCD_Display_Init(void)
{
    // LCD外设已在main.c中初始化
    
    // 使能LCD
    __HAL_LCD_ENABLE(&hlcd);
    
    // 清屏
    HAL_LCD_Clear(&hlcd);
    
    // 测试：显示"123456"验证所有数字位
    for (uint8_t pos = 0; pos < 6; pos++) {
        LCD_Write_Digit(pos, pos + 1);
    }
    
    // 测试所有符号显示
    LCD_Display_Symbol(LCD_SYMBOL_BATTERY, 1);      // 电池符号
    LCD_Display_Symbol(LCD_SYMBOL_UNIT_MM, 1);      // mm单位
    LCD_Display_Symbol(LCD_SYMBOL_UNIT_INCH, 1);    // inch单位
    LCD_Display_Symbol(LCD_SYMBOL_NEGATIVE, 1);     // 负号
    LCD_Display_Symbol(LCD_SYMBOL_POSITIVE, 1);     // 正号
    LCD_Display_Symbol(LCD_SYMBOL_FWD, 1);          // 前进符号
    LCD_Display_Symbol(LCD_SYMBOL_REV, 1);          // 后退符号
    
    // 测试更多符号
    LCD_Display_Symbol(LCD_SYMBOL_SET, 1);          // set符号
    LCD_Display_Symbol(LCD_SYMBOL_OK, 1);           // ok符号
    LCD_Display_Symbol(LCD_SYMBOL_NG, 1);           // NG符号
    LCD_Display_Symbol(LCD_SYMBOL_TOL, 1);          // tol符号
    LCD_Display_Symbol(LCD_SYMBOL_REL, 1);          // rel符号
    LCD_Display_Symbol(LCD_SYMBOL_MAX, 1);          // max符号
    LCD_Display_Symbol(LCD_SYMBOL_TIR, 1);          // TIR符号
    LCD_Display_Symbol(LCD_SYMBOL_MIN, 1);          // min符号
    LCD_Display_Symbol(LCD_SYMBOL_HOLD, 1);         // hold符号
    LCD_Display_Symbol(LCD_SYMBOL_INS1, 1);         // ins1符号
    LCD_Display_Symbol(LCD_SYMBOL_INU1, 1);         // inu1符号
    LCD_Display_Symbol(LCD_SYMBOL_IN_P, 1);         // in-p符号
    LCD_Display_Symbol(LCD_SYMBOL_MM_P, 1);         // mm-p符号
    LCD_Display_Symbol(LCD_SYMBOL_MMST, 1);         // mmst符号
    LCD_Display_Symbol(LCD_SYMBOL_MMUT, 1);         // mmut符号
    
    // 测试小数点（在数字2和3后面显示小数点）
    LCD_Display_Dot(1, 1);  // 数字2后面的小数点
    LCD_Display_Dot(2, 1);  // 数字3后面的小数点
    
    // 测试所有ps电源档位符号 (ps0~ps31)
    LCD_Display_Symbol(LCD_SYMBOL_PS_0, 1);      // ps0
    LCD_Display_Symbol(LCD_SYMBOL_PS_PP, 1);     // ps++
    LCD_Display_Symbol(LCD_SYMBOL_PS_P15, 1);    // ps+15
    LCD_Display_Symbol(LCD_SYMBOL_PS_P14, 1);    // ps+14
    LCD_Display_Symbol(LCD_SYMBOL_PS_P13, 1);    // ps+13
    LCD_Display_Symbol(LCD_SYMBOL_PS_P12, 1);    // ps+12
    LCD_Display_Symbol(LCD_SYMBOL_PS_P11, 1);    // ps+11
    LCD_Display_Symbol(LCD_SYMBOL_PS_P10, 1);    // ps+10
    LCD_Display_Symbol(LCD_SYMBOL_PS_P9, 1);     // ps+9
    LCD_Display_Symbol(LCD_SYMBOL_PS_P8, 1);     // ps+8
    LCD_Display_Symbol(LCD_SYMBOL_PS_P7, 1);     // ps+7
    LCD_Display_Symbol(LCD_SYMBOL_PS_P6, 1);     // ps+6
    LCD_Display_Symbol(LCD_SYMBOL_PS_P5, 1);     // ps+5
    LCD_Display_Symbol(LCD_SYMBOL_PS_P4, 1);     // ps+4
    LCD_Display_Symbol(LCD_SYMBOL_PS_P3, 1);     // ps+3
    LCD_Display_Symbol(LCD_SYMBOL_PS_P2, 1);     // ps+2
    LCD_Display_Symbol(LCD_SYMBOL_PS_P1, 1);     // ps+1
    LCD_Display_Symbol(LCD_SYMBOL_PS_N1, 1);     // ps-1
    LCD_Display_Symbol(LCD_SYMBOL_PS_N2, 1);     // ps-2
    LCD_Display_Symbol(LCD_SYMBOL_PS_N3, 1);     // ps-3
    LCD_Display_Symbol(LCD_SYMBOL_PS_N4, 1);     // ps-4
    LCD_Display_Symbol(LCD_SYMBOL_PS_N5, 1);     // ps-5
    LCD_Display_Symbol(LCD_SYMBOL_PS_N6, 1);     // ps-6
    LCD_Display_Symbol(LCD_SYMBOL_PS_N7, 1);     // ps-7
    LCD_Display_Symbol(LCD_SYMBOL_PS_N8, 1);     // ps-8
    LCD_Display_Symbol(LCD_SYMBOL_PS_N9, 1);     // ps-9
    LCD_Display_Symbol(LCD_SYMBOL_PS_N10, 1);    // ps-10
    LCD_Display_Symbol(LCD_SYMBOL_PS_N11, 1);    // ps-11
    LCD_Display_Symbol(LCD_SYMBOL_PS_N12, 1);    // ps-12
    LCD_Display_Symbol(LCD_SYMBOL_PS_N13, 1);    // ps-13
    LCD_Display_Symbol(LCD_SYMBOL_PS_N14, 1);    // ps-14
    LCD_Display_Symbol(LCD_SYMBOL_PS_NN, 1);     // ps--
    LCD_Display_Symbol(LCD_SYMBOL_PS_N15, 1);    // ps-15
    
    // 更新显示
    HAL_LCD_UpdateDisplayRequest(&hlcd);
}

/**
  * @brief  清除LCD显示
  * @retval None
  */
void LCD_Display_Clear(void)
{
    HAL_LCD_Clear(&hlcd);
}

/**
  * @brief  显示数字
  * @param  number: 要显示的数字
  * @param  decimal_places: 小数位数
  * @retval None
  */
void LCD_Display_Number(float number, uint8_t decimal_places)
{
/*     uint8_t digits[MAX_DIGITS];
    uint8_t digit_count = 0;
    
    // 转换数字为各位数字
    Convert_NumberToDigits(number, decimal_places, digits, &digit_count);
    
    // 显示各位数字
    for (uint8_t i = 0; i < digit_count && i < MAX_DIGITS; i++) {
        LCD_Write_Digit(i, digits[i]);
    }
    
    // 更新显示
    LCD_Display_Update(); */
    uint8_t digits[MAX_DIGITS];
    uint8_t digit_count = 0;
    
    // 转换数字为各位数字
    Convert_NumberToDigits(number, decimal_places, digits, &digit_count);
    
    // 显示各位数字
    for (uint8_t i = 0; i < digit_count && i < MAX_DIGITS; i++) {
        LCD_Write_Digit(i, digits[i]);
    }
    
    // 清除所有可能的小数点，防止切换分辨率时小数点残留
    for (uint8_t i = 0; i < 5; i++) {
        LCD_Display_Dot(i, 0);
    }
    
    // 在适当位置显示小数点
    if (decimal_places > 0 && decimal_places < MAX_DIGITS) {
        // 小数点位置：MAX_DIGITS - 1 - decimal_places
        // 例如6位数字，2位小数，小数点在第3位后面（索引为3）
        LCD_Display_Dot(MAX_DIGITS - 1 - decimal_places, 1);
    }
    
    // 可选：在这里统一处理负号显示
     LCD_Display_Symbol(LCD_SYMBOL_NEGATIVE, (number < 0) ? 1 : 0);
    
    // 更新显示
    LCD_Display_Update();
}

/**
  * @brief  显示字符串
  * @param  str: 字符串指针
  * @retval None
  * @note   根据实际LCD支持的字符调整
  */
void LCD_Display_String(char *str)
{
    // TODO: 实现字符串显示
    // 需要根据LCD段码定义实现字符映射
}

/**
  * @brief  显示符号
  * @param  symbol: 符号编号
  * @param  state: 0-关闭, 1-开启
  * @retval None
  * @note   根据用户提供的SEG/COM映射表
  */
void LCD_Display_Symbol(uint8_t symbol, uint8_t state)
{
    // 根据用户提供的映射表
    switch(symbol) {
        case LCD_SYMBOL_BATTERY:
            // 'lo-batt' at SEG17 COM1 (注意：COM从0开始，表格COM1=代码COM0)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<17), (uint32_t)(state<<17));
            break;
            
        case LCD_SYMBOL_UNIT_MM:
            // 'mm' at SEG16 COM1
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<16), (uint32_t)(state<<16));
            break;
            
        case LCD_SYMBOL_UNIT_INCH:
            // 'in' at SEG15 COM4 (COM3)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<15), (uint32_t)(state<<15));
            break;
            
        case LCD_SYMBOL_NEGATIVE:
            // 'sign' at SEG0 COM3 (COM2)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<0), (uint32_t)(state<<0));
            break;
            
        case LCD_SYMBOL_POSITIVE:
            // '+sign' at SIG0 COM4 (代码COM3)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<0), (uint32_t)(state<<0));
            break;
            
        case LCD_SYMBOL_FWD:
            // 'fwd' at SEG0 COM2 (COM1)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<0), (uint32_t)(state<<0));
            break;
            
        case LCD_SYMBOL_REV:
            // 'rev' at SIG0 COM1 (代码COM0)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<0), (uint32_t)(state<<0));
            break;
            
        case LCD_SYMBOL_DOT_1:
        case LCD_SYMBOL_DOT_2:
        case LCD_SYMBOL_DOT_3:
        case LCD_SYMBOL_DOT_4:
        case LCD_SYMBOL_DOT_5:
            // 小数点通过LCD_Display_Dot函数处理
            LCD_Display_Dot(symbol - LCD_SYMBOL_DOT_1, state);
            return; // 已经更新显示，直接返回
            
        case LCD_SYMBOL_SET:
            // 'set' at SIG11 COM4 (代码COM3) - 根据新断码表
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<11), (uint32_t)(state<<11));
            break;
            
        case LCD_SYMBOL_OK:
            // 'ok' at SIG12 COM3 (代码COM2) 或 SIG13 COM1 (代码COM0)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<12), (uint32_t)(state<<12));
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<13), (uint32_t)(state<<13));
            break;
            
        case LCD_SYMBOL_NG:
            // 'NG' at SIG12 COM4 (代码COM3) 或 SIG13 COM2 (代码COM1)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<12), (uint32_t)(state<<12));
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<13), (uint32_t)(state<<13));
            break;
            
        case LCD_SYMBOL_TOL:
            // 'tol' at SIG13 COM3 (代码COM2)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<13), (uint32_t)(state<<13));
            break;
            
        case LCD_SYMBOL_REL:
            // 'rel' at SIG13 COM4 (代码COM3)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<13), (uint32_t)(state<<13));
            break;
            
        case LCD_SYMBOL_MAX:
            // 'max' at SIG14 COM1 (代码COM0)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<14), (uint32_t)(state<<14));
            break;
            
        case LCD_SYMBOL_TIR:
            // 'TIR' at SIG14 COM2 (代码COM1) - 新断码表中是'ins1'，可能需要确认
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<14), (uint32_t)(state<<14));
            break;
            
        case LCD_SYMBOL_MIN:
            // 'min' at SIG14 COM3 (代码COM2)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<14), (uint32_t)(state<<14));
            break;
            
        case LCD_SYMBOL_HOLD:
            // 'hold' at SIG14 COM4 (代码COM3)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<14), (uint32_t)(state<<14));
            break;
            
        case LCD_SYMBOL_INS1:
            // 'ins1' at SIG14 COM2 (代码COM1) 和 SIG15 COM1 (代码COM0)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<14), (uint32_t)(state<<14));
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<15), (uint32_t)(state<<15));
            break;
            
        case LCD_SYMBOL_INU1:
            // 'inu1' at SIG15 COM2 (代码COM1)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<15), (uint32_t)(state<<15));
            break;
            
        case LCD_SYMBOL_IN_P:
            // 'in-p' at SIG15 COM3 (代码COM2)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<15), (uint32_t)(state<<15));
            break;
            
        case LCD_SYMBOL_MM_P:
            // 'mm-p' at SIG16 COM2 (代码COM1) 和 'mm+p' at SIG16 COM3 (代码COM2)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<16), (uint32_t)(state<<16));
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<16), (uint32_t)(state<<16));
            break;
            
        case LCD_SYMBOL_MMST:
            // 'mmst' - 新断码表中未找到，保持原映射
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<16), (uint32_t)(state<<16));
            break;
            
        case LCD_SYMBOL_MMUT:
            // 'mmut' at SEG16 COM4 (COM3)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<16), (uint32_t)(state<<16));
            break;
            
        // SEG17 电源相关符号
        case LCD_SYMBOL_PS_PP:
            // 'ps++' at SEG17 COM2 (COM1)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<17), (uint32_t)(state<<17));
            break;
        case LCD_SYMBOL_PS_P15:
            // 'ps+15' at SEG17 COM3 (COM2)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<17), (uint32_t)(state<<17));
            break;
        case LCD_SYMBOL_PS_0:
            // 'ps0' at SEG17 COM4 (COM3)
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<17), (uint32_t)(state<<17));
            break;
            
        // SEG18 电源档位
        case LCD_SYMBOL_PS_P14:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<18), (uint32_t)(state<<18));
            break;
        case LCD_SYMBOL_PS_P13:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<18), (uint32_t)(state<<18));
            break;
        case LCD_SYMBOL_PS_P12:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<18), (uint32_t)(state<<18));
            break;
        case LCD_SYMBOL_PS_P11:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<18), (uint32_t)(state<<18));
            break;
            
        // SEG19 电源档位
        case LCD_SYMBOL_PS_P10:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<19), (uint32_t)(state<<19));
            break;
        case LCD_SYMBOL_PS_P9:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<19), (uint32_t)(state<<19));
            break;
        case LCD_SYMBOL_PS_P8:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<19), (uint32_t)(state<<19));
            break;
        case LCD_SYMBOL_PS_P7:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<19), (uint32_t)(state<<19));
            break;
            
        // SEG20 电源档位
        case LCD_SYMBOL_PS_P6:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<20), (uint32_t)(state<<20));
            break;
        case LCD_SYMBOL_PS_P5:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<20), (uint32_t)(state<<20));
            break;
        case LCD_SYMBOL_PS_P4:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<20), (uint32_t)(state<<20));
            break;
        case LCD_SYMBOL_PS_P3:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<20), (uint32_t)(state<<20));
            break;
            
        // SEG21 电源档位
        case LCD_SYMBOL_PS_P2:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<21), (uint32_t)(state<<21));
            break;
        case LCD_SYMBOL_PS_P1:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<21), (uint32_t)(state<<21));
            break;
        case LCD_SYMBOL_PS_N1:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<21), (uint32_t)(state<<21));
            break;
        case LCD_SYMBOL_PS_N2:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<21), (uint32_t)(state<<21));
            break;
            
        // SEG22 电源档位
        case LCD_SYMBOL_PS_N3:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<22), (uint32_t)(state<<22));
            break;
        case LCD_SYMBOL_PS_N4:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<22), (uint32_t)(state<<22));
            break;
        case LCD_SYMBOL_PS_N5:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<22), (uint32_t)(state<<22));
            break;
        case LCD_SYMBOL_PS_N6:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<22), (uint32_t)(state<<22));
            break;
            
        // SEG23 电源档位
        case LCD_SYMBOL_PS_N7:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<23), (uint32_t)(state<<23));
            break;
        case LCD_SYMBOL_PS_N8:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<23), (uint32_t)(state<<23));
            break;
        case LCD_SYMBOL_PS_N9:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<23), (uint32_t)(state<<23));
            break;
        case LCD_SYMBOL_PS_N10:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<23), (uint32_t)(state<<23));
            break;
            
        // SEG24 电源档位
        case LCD_SYMBOL_PS_N11:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<24), (uint32_t)(state<<24));
            break;
        case LCD_SYMBOL_PS_N12:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<24), (uint32_t)(state<<24));
            break;
        case LCD_SYMBOL_PS_N13:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, (uint32_t)~(1<<24), (uint32_t)(state<<24));
            break;
        case LCD_SYMBOL_PS_N14:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, (uint32_t)~(1<<24), (uint32_t)(state<<24));
            break;
            
        // SEG25 电源档位
        case LCD_SYMBOL_PS_NN:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, (uint32_t)~(1<<25), (uint32_t)(state<<25));
            break;
        case LCD_SYMBOL_PS_N15:
            HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, (uint32_t)~(1<<25), (uint32_t)(state<<25));
            break;
            
        default:
            break;
    }
    // 注意：不在此处调用UpdateDisplayRequest，由调用方统一更新，避免中间状态鬼影
}

/**
  * @brief  显示电池图标
  * @param  show: 0-隐藏, 1-显示
  * @retval None
  * @note   用于电池低压指示
  */
void LCD_Display_BatteryIcon(uint8_t show)
{
    LCD_Display_Symbol(LCD_SYMBOL_BATTERY, show);
}

/**
  * @brief  更新LCD显示
  * @retval None
  */
void LCD_Display_Update(void)
{
    HAL_LCD_UpdateDisplayRequest(&hlcd);
}

/**
  * @brief  写入段码
  * @param  com: COM口编号(0-3)
  * @param  seg: 段编号(0-26)
  * @param  state: 0-关闭, 1-开启
  * @retval None
  */
void LCD_Write_Segment(uint8_t com, uint8_t seg, uint8_t state)
{
    // 使用HAL库函数写入段码
    // 根据STM32L1的LCD寄存器映射实现
    
    // TODO: 实现具体的段码写入
    // 需要根据实际硬件连接和寄存器定义实现
    
    if (state) {
        HAL_LCD_Write(&hlcd, com, seg, 1);
    } else {
        HAL_LCD_Write(&hlcd, com, seg, 0);
    }
}

/**
  * @brief  显示单个数字
  * @param  position: 显示位置 (0-5, 从左到右，对应数字1-6)
  * @param  digit: 数字(0-9)
  * @retval None
  * @note   根据用户提供的SEG/COM映射表
  */
/**
  * @brief  显示小数点
  * @param  position: 小数点位置 (0-4, 对应数字1-5后面)
  * @param  state: 0-关闭, 1-开启
  * @retval None
  * @note   小数点在每个数字的SEG+1位置的bit1
  */
void LCD_Display_Dot(uint8_t position, uint8_t state)
{
    if (position >= 5) {
        return;
    }
    
    // 根据映射表：小数点p1-p5在SEG2,4,6,8,10的COM4(COM3)
    // p1: SEG2 COM4, p2: SEG4 COM4, p3: SEG6 COM4, p4: SEG8 COM4, p5: SEG10 COM4
    uint8_t seg_positions[5] = {2, 4, 6, 8, 10};
    uint8_t seg = seg_positions[position];
    
    // 小数点在COM4 (RAM_REGISTER6)
    HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, 
                  (uint32_t)~(1 << seg), 
                  (uint32_t)(state << seg));
    // 注意：不在此处调用UpdateDisplayRequest，由调用方统一更新
}

/**
  * @brief  显示单个数字
  * @param  position: 显示位置 (0-5, 从左到右，对应数字1-6)
  * @param  digit: 数字(0-9)
  * @retval None
  * @note   根据用户提供的SEG/COM映射表
  */
void LCD_Write_Digit(uint8_t position, uint8_t digit)
{
    if (digit > 9 || position >= 6) {
        return;
    }

    // 获取该数字的起始SEG位置
    uint8_t seg_start = DISP_NUM[position];
    
    // 按4行(COM0-COM3)写入数字段码
    // 根据CSDN文章，HAL_LCD_Write需要4个参数：
    // 参数1: hlcd句柄
    // 参数2: RAM寄存器编号
    // 参数3: 清除掩码 (取反)
    // 参数4: 要写入的值
    
    // 写入COM0 (RAM_REGISTER0) - 权值为3 (F*1 + A*2)
    uint8_t val0 = DISP_NUM_TAB[digit][0];
    HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, 
                  (uint32_t)~(3 << seg_start), 
                  (uint32_t)(val0 << seg_start));
    
    // 写入COM1 (RAM_REGISTER2) - 权值为3 (G*1 + B*2)
    uint8_t val1 = DISP_NUM_TAB[digit][1];
    HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, 
                  (uint32_t)~(3 << seg_start), 
                  (uint32_t)(val1 << seg_start));
    
    // 写入COM2 (RAM_REGISTER4) - 权值为3 (E*1 + C*2)
    uint8_t val2 = DISP_NUM_TAB[digit][2];
    HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, 
                  (uint32_t)~(3 << seg_start), 
                  (uint32_t)(val2 << seg_start));
    
    // 写入COM3 (RAM_REGISTER6) - 权值为1 (D*1)
    uint8_t val3 = DISP_NUM_TAB[digit][3];
    HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, 
                  (uint32_t)~(1 << seg_start), 
                  (uint32_t)(val3 << seg_start));
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  将数字转换为各位数字数组
  * @param  number: 输入数字
  * @param  decimal_places: 小数位数
  * @param  digits: 输出数字数组
  * @param  digit_count: 输出数字个数
  * @retval None
  */
static void Convert_NumberToDigits(float number, uint8_t decimal_places, uint8_t *digits, uint8_t *digit_count)
{
/*     int32_t int_part;
    int32_t frac_part;
    uint8_t index = 0;
    
    // 处理负数
    if (number < 0) {
        number = -number;
        // TODO: 显示负号
    }
    
    // 分离整数和小数部分
    int_part = (int32_t)number;
    frac_part = (int32_t)((number - int_part) * pow(10, decimal_places));
    
    // 转换整数部分
    if (int_part == 0) {
        digits[index++] = 0;
    } else {
        uint8_t temp_digits[MAX_DIGITS];
        uint8_t temp_count = 0;
        
        while (int_part > 0 && temp_count < MAX_DIGITS) {
            temp_digits[temp_count++] = int_part % 10;
            int_part /= 10;
        }
        
        // 反转顺序
        for (int8_t i = temp_count - 1; i >= 0; i--) {
            digits[index++] = temp_digits[i];
        }
    }
    
    // 添加小数点位置标记
    // TODO: 在适当位置显示小数点
    
    // 转换小数部分
    if (decimal_places > 0) {
        for (uint8_t i = 0; i < decimal_places && index < MAX_DIGITS; i++) {
            digits[index++] = frac_part % 10;
            frac_part /= 10;
        }
    }
    
    *digit_count = index; */

    // 处理负数
    if (number < 0) {
        number = -number;
    }
    
    // 将浮点数放大为整数，加0.5f是为了防止浮点数精度问题导致的向下取整
    uint32_t total_val = (uint32_t)(number * pow(10, decimal_places) + 0.5f);
    
    // 固定提取6位数字（从右到左，即从低位到高位）
    // digits[0] 是最左边的最高位，digits[5] 是最右边的最低位
    for (int8_t i = MAX_DIGITS - 1; i >= 0; i--) {
        digits[i] = total_val % 10;
        total_val /= 10;
    }
    
    *digit_count = MAX_DIGITS; // 固定输出6位
}
