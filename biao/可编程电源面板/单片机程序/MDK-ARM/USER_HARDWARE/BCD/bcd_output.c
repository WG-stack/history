#include "bcd_output.h"

static uint8_t bcd_output_value = 0;  // 当前BCD输出值

/**
 * @brief BCD输出初始化
 */
void BCD_Init(void)
{
    BCD_SetOutput(0);
    BCD_COM_Disable();
}

/**
 * @brief 设置BCD输出值(7位)
 * @param value 输出值 (bit0-bit6对应BCD_OUT0-BCD_OUT6)
 * @note BCD_OUT0-PC12, BCD_OUT1-PD2, BCD_OUT2-PB3, BCD_OUT3-PB4
 *       BCD_OUT4-PB5, BCD_OUT5-PB6, BCD_OUT6-PB7
 */
void BCD_SetOutput(uint8_t value)
{
    bcd_output_value = value;
    
    PCout(12) = (value & 0x01) ? 1 : 0;  // BCD_OUT0
    PDout(2)  = (value & 0x02) ? 1 : 0;  // BCD_OUT1
    PBout(3)  = (value & 0x04) ? 1 : 0;  // BCD_OUT2
    PBout(4)  = (value & 0x08) ? 1 : 0;  // BCD_OUT3
    PBout(5)  = (value & 0x10) ? 1 : 0;  // BCD_OUT4
    PBout(6)  = (value & 0x20) ? 1 : 0;  // BCD_OUT5
    PBout(7)  = (value & 0x40) ? 1 : 0;  // BCD_OUT6
}

/**
 * @brief 设置单个BCD输出位
 * @param bit 位号 (0-6)
 * @param state 状态 (0-低电平, 1-高电平)
 */
void BCD_SetBit(uint8_t bit, uint8_t state)
{
    if(bit > 6)
        return;
    
    if(state)
        bcd_output_value |= (1 << bit);
    else
        bcd_output_value &= ~(1 << bit);
    
    BCD_SetOutput(bcd_output_value);
}

/**
 * @brief 获取当前BCD输出值
 * @return 当前输出值
 */
uint8_t BCD_GetOutput(void)
{
    return bcd_output_value;
}

/**
 * @brief 使能BCD公共端
 * @note BCD_COM引脚为PB8
 */
void BCD_COM_Enable(void)
{
    PBout(8) = 1;
}

/**
 * @brief 禁用BCD公共端
 * @note BCD_COM引脚为PB8
 */
void BCD_COM_Disable(void)
{
    PBout(8) = 0;
}
