/**
  ******************************************************************************
  * @file           : battery_monitor.c
  * @brief          : 电池电压检测模块实现
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "battery_monitor.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define ADC_SAMPLE_COUNT    10  // ADC采样次数（滤波）

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static BatteryInfo_t battery_info;
static uint16_t adc_buffer[ADC_SAMPLE_COUNT];
static uint8_t adc_index = 0;
static uint32_t blink_timer = 0;    // 闪烁定时器

/* Private function prototypes -----------------------------------------------*/
static float Calculate_Voltage(uint16_t adc_value);
static uint8_t Calculate_Percentage(float voltage);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化电池监测模块
  * @retval None
  */
void Battery_Init(void)
{
    memset(&battery_info, 0, sizeof(BatteryInfo_t));
    memset(adc_buffer, 0, sizeof(adc_buffer));
    adc_index = 0;
    
    // 启动ADC
    HAL_ADC_Start(&hadc);
}

/**
  * @brief  更新电池信息
  * @note   建议在主循环或定时器中周期调用
  * @retval None
  */
void Battery_Update(void)
{
    uint16_t adc_value;
    uint32_t adc_sum = 0;
    
    // 读取ADC值
    if (HAL_ADC_PollForConversion(&hadc, 10) == HAL_OK) {
        adc_value = HAL_ADC_GetValue(&hadc);
        
        // 滤波：滚动平均
        adc_buffer[adc_index] = adc_value;
        adc_index = (adc_index + 1) % ADC_SAMPLE_COUNT;
        
        // 计算平均值
        for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++) {
            adc_sum += adc_buffer[i];
        }
        adc_value = adc_sum / ADC_SAMPLE_COUNT;
        
        // 更新电池信息
        battery_info.adc_value = adc_value;
        battery_info.voltage = Calculate_Voltage(adc_value);
        battery_info.percentage = Calculate_Percentage(battery_info.voltage);
        
        // 判断电池状态
        if (battery_info.voltage < BATTERY_LOW_CRITICAL) {
            battery_info.status = BATTERY_LOW_CRITICAL;
            // 闪烁控制（500ms周期）
            if (HAL_GetTick() - blink_timer >= 500) {
                battery_info.icon_blink = !battery_info.icon_blink;
                blink_timer = HAL_GetTick();
            }
        } else if (battery_info.voltage < BATTERY_LOW_WARNING) {
            battery_info.status = BATTERY_LOW_WARNING;
            battery_info.icon_blink = 0;  // 常显，不闪烁
        } else {
            battery_info.status = BATTERY_NORMAL;
            battery_info.icon_blink = 0;
        }
    }
    
    // 重新启动ADC
    HAL_ADC_Start(&hadc);
}

/**
  * @brief  获取电池信息
  * @param  info: 电池信息结构指针
  * @retval None
  */
void Battery_GetInfo(BatteryInfo_t *info)
{
    if (info != NULL) {
        memcpy(info, &battery_info, sizeof(BatteryInfo_t));
    }
}

/**
  * @brief  获取电池状态
  * @retval 电池状态枚举
  */
BatteryStatus_t Battery_GetStatus(void)
{
    return battery_info.status;
}

/**
  * @brief  获取电池电压
  * @retval 电池电压(V)
  */
float Battery_GetVoltage(void)
{
    return battery_info.voltage;
}

/**
  * @brief  判断电池图标是否应该闪烁
  * @retval 0-不闪烁（常显或不显示）, 1-当前应显示, 0-当前应隐藏
  * @note   仅在电压低于2.6V时返回闪烁状态
  */
uint8_t Battery_ShouldBlinkIcon(void)
{
    if (battery_info.status == BATTERY_LOW_CRITICAL) {
        return battery_info.icon_blink;
    }
    return 1;  // 其他状态常显
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  计算电池电压
  * @param  adc_value: ADC原始值
  * @retval 电压值(V)
  */
static float Calculate_Voltage(uint16_t adc_value)
{
    float voltage;
    
    // ADC值转换为电压
    voltage = ((float)adc_value / ADC_RESOLUTION) * ADC_VREF;
    
    // 考虑分压电路
    voltage = voltage * BATTERY_DIVIDER;
    
    return voltage;
}

/**
  * @brief  计算电量百分比
  * @param  voltage: 电池电压
  * @retval 电量百分比(0-100)
  */
static uint8_t Calculate_Percentage(float voltage)
{
    // 根据实际电池特性曲线计算
    // 使用2节AA电池（或等效电压范围）
    uint8_t percentage;
    
    if (voltage >= BATTERY_FULL_VOLTAGE) {
        percentage = 100;
    } else if (voltage <= BATTERY_EMPTY_VOLTAGE) {
        percentage = 0;
    } else {
        percentage = (uint8_t)(((voltage - BATTERY_EMPTY_VOLTAGE) / 
                               (BATTERY_FULL_VOLTAGE - BATTERY_EMPTY_VOLTAGE)) * 100);
    }
    
    return percentage;
}
