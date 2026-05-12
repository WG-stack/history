/**
  ******************************************************************************
  * @file           : battery_monitor.h
  * @brief          : 电池电压检测模块头文件
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

#ifndef __BATTERY_MONITOR_H
#define __BATTERY_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    BATTERY_NORMAL = 0,         // 正常
    BATTERY_LOW_WARNING,        // 低压警告（2.8V以下，常显）
    BATTERY_LOW_CRITICAL        // 低压严重（2.6V以下，闪动）
} BatteryStatus_t;

typedef struct {
    uint16_t adc_value;         // ADC原始值
    float    voltage;           // 电池电压(V)
    uint8_t  percentage;        // 电量百分比(0-100)
    BatteryStatus_t status;     // 电池状态
    uint8_t  icon_blink;        // 电池图标闪烁标志（用于2.6V以下）
} BatteryInfo_t;

/* Exported constants --------------------------------------------------------*/
#define ADC_VREF                3.3f    // ADC参考电压
#define ADC_RESOLUTION          4096    // 12位ADC分辨率
#define BATTERY_DIVIDER         2.0f    // 分压比（R11=1M, R14=1M，分压比=2）
#define BATTERY_LOW_WARNING     2.8f    // 低压警告阈值（常显电池符号）
#define BATTERY_LOW_CRITICAL    2.6f    // 低压严重阈值（闪动电池符号）
#define BATTERY_FULL_VOLTAGE    3.0f    // 满电电压
#define BATTERY_EMPTY_VOLTAGE   2.4f    // 空电电压

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void Battery_Init(void);
void Battery_Update(void);
void Battery_GetInfo(BatteryInfo_t *info);
BatteryStatus_t Battery_GetStatus(void);
float Battery_GetVoltage(void);
uint8_t Battery_ShouldBlinkIcon(void);

#ifdef __cplusplus
}
#endif

#endif /* __BATTERY_MONITOR_H */
