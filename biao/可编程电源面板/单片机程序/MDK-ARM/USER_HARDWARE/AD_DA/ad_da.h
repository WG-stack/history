#ifndef _AD_DA_H_
#define _AD_DA_H_

#include "main.h"
#include "adc.h"
#include "dac.h"

#define ADC_CHANNEL_NUM  2  // ADC通道数

/* ADC数据结构体 */
typedef struct {
    uint16_t adc_raw[ADC_CHANNEL_NUM];  // ADC原始值 [0]-电流, [1]-电压
    float current_in;                    // 电流输入值 (0-5A)
    float voltage_in;                    // 电压输入值 (100-400V)
} ADC_Data_t;

/* DAC数据结构体 */
typedef struct {
    uint16_t dac_raw_iout;  // 电流输出DAC原始值
    uint16_t dac_raw_vout;  // 电压输出DAC原始值
    float current_out;       // 电流输出值 (4-20mA)
    float voltage_out;       // 电压输出值 (1-5V)
} DAC_Data_t;

extern ADC_Data_t ADC_Data;  // ADC数据
extern DAC_Data_t DAC_Data;  // DAC数据

/**
 * @brief AD/DA模块初始化
 */
void AD_DA_Init(void);

/**
 * @brief 启动ADC DMA采样
 */
void ADC_Start(void);

/**
 * @brief 更新ADC转换后的物理量
 */
void ADC_Update(void);

/**
 * @brief 获取电流输入值
 * @return 电流值 (A)
 */
float ADC_GetCurrent(void);

/**
 * @brief 获取电压输入值
 * @return 电压值 (V)
 */
float ADC_GetVoltage(void);

/**
 * @brief 获取电流ADC原始值
 * @return ADC原始值 (0-4095)
 */
uint16_t ADC_GetRawCurrent(void);

/**
 * @brief 获取电压ADC原始值
 * @return ADC原始值 (0-4095)
 */
uint16_t ADC_GetRawVoltage(void);

/**
 * @brief 设置电流输出
 * @param current_ma 电流值 (4-20mA)
 */
void DAC_SetCurrent(float current_ma);

/**
 * @brief 设置电压输出
 * @param voltage_v 电压值 (1-5V)
 */
void DAC_SetVoltage(float voltage_v);

/**
 * @brief 直接设置电流输出DAC值
 * @param dac_value DAC值 (0-4095)
 */
void DAC_SetRawCurrent(uint16_t dac_value);

/**
 * @brief 直接设置电压输出DAC值
 * @param dac_value DAC值 (0-4095)
 */
void DAC_SetRawVoltage(uint16_t dac_value);

/**
 * @brief 获取当前电流输出值
 * @return 电流值 (mA)
 */
float DAC_GetCurrentOutput(void);

/**
 * @brief 获取当前电压输出值
 * @return 电压值 (V)
 */
float DAC_GetVoltageOutput(void);

#endif
