#include "ad_da.h"

ADC_Data_t ADC_Data = {0};  // ADC数据全局变量
DAC_Data_t DAC_Data = {0};  // DAC数据全局变量

/**
 * @brief AD/DA模块初始化
 * @note 启动DAC通道，设置默认输出值，启动ADC DMA采样
 */
void AD_DA_Init(void)
{
    HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
    HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
    
    DAC_SetCurrent(4.0f);
    DAC_SetVoltage(1.0f);
    
    ADC_Start();
}

/**
 * @brief 启动ADC DMA采样
 * @note PA6(ADC1_IN6)-电流采样, PA7(ADC1_IN7)-电压采样
 */
void ADC_Start(void)
{
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC_Data.adc_raw, ADC_CHANNEL_NUM);
}

/**
 * @brief 更新ADC转换后的物理量
 * @note 将ADC原始值转换为实际电流和电压值
 */
void ADC_Update(void)
{
    ADC_Data.current_in = ADC_GetCurrent();
    ADC_Data.voltage_in = ADC_GetVoltage();
}

/**
 * @brief 获取电流输入值
 * @return 电流值 (A)
 * @note 转换关系式待定，当前为占位实现
 */
float ADC_GetCurrent(void)
{
    float current;
    current = (float)ADC_Data.adc_raw[0];
    return current;
}

/**
 * @brief 获取电压输入值
 * @return 电压值 (V)
 * @note 转换关系式待定，当前为占位实现
 */
float ADC_GetVoltage(void)
{
    float voltage;
    voltage = (float)ADC_Data.adc_raw[1];
    return voltage;
}

/**
 * @brief 获取电流ADC原始值
 * @return ADC原始值 (0-4095)
 */
uint16_t ADC_GetRawCurrent(void)
{
    return ADC_Data.adc_raw[0];
}

/**
 * @brief 获取电压ADC原始值
 * @return ADC原始值 (0-4095)
 */
uint16_t ADC_GetRawVoltage(void)
{
    return ADC_Data.adc_raw[1];
}

/**
 * @brief 设置电流输出 (4-20mA)
 * @param current_ma 电流值 (mA)
 * @note PA4(DAC_OUT1), 输出范围4-20mA
 */
void DAC_SetCurrent(float current_ma)
{
    uint16_t dac_value;
    
    if(current_ma < 4.0f)
        current_ma = 4.0f;
    if(current_ma > 20.0f)
        current_ma = 20.0f;
    
    DAC_Data.current_out = current_ma;
    dac_value = (uint16_t)((current_ma - 4.0f) / 16.0f * 4095.0f);
    DAC_Data.dac_raw_iout = dac_value;
    
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
}

/**
 * @brief 设置电压输出 (1-5V)
 * @param voltage_v 电压值 (V)
 * @note PA5(DAC_OUT2), 输出范围1-5V
 */
void DAC_SetVoltage(float voltage_v)
{
    uint16_t dac_value;
    
    if(voltage_v < 1.0f)
        voltage_v = 1.0f;
    if(voltage_v > 5.0f)
        voltage_v = 5.0f;
    
    DAC_Data.voltage_out = voltage_v;
    dac_value = (uint16_t)((voltage_v - 1.0f) / 4.0f * 4095.0f);
    DAC_Data.dac_raw_vout = dac_value;
    
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
}

/**
 * @brief 直接设置电流输出DAC值
 * @param dac_value DAC值 (0-4095)
 */
void DAC_SetRawCurrent(uint16_t dac_value)
{
    if(dac_value > 4095)
        dac_value = 4095;
    
    DAC_Data.dac_raw_iout = dac_value;
    DAC_Data.current_out = (float)dac_value / 4095.0f * 16.0f + 4.0f;
    
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
}

/**
 * @brief 直接设置电压输出DAC值
 * @param dac_value DAC值 (0-4095)
 */
void DAC_SetRawVoltage(uint16_t dac_value)
{
    if(dac_value > 4095)
        dac_value = 4095;
    
    DAC_Data.dac_raw_vout = dac_value;
    DAC_Data.voltage_out = (float)dac_value / 4095.0f * 4.0f + 1.0f;
    
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
}

/**
 * @brief 获取当前电流输出值
 * @return 电流值 (mA)
 */
float DAC_GetCurrentOutput(void)
{
    return DAC_Data.current_out;
}

/**
 * @brief 获取当前电压输出值
 * @return 电压值 (V)
 */
float DAC_GetVoltageOutput(void)
{
    return DAC_Data.voltage_out;
}
