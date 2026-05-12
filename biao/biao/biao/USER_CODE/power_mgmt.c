/**
  ******************************************************************************
  * @file           : power_mgmt.c
  * @brief          : 电源管理模块实现
  ******************************************************************************
  */

#include "power_mgmt.h"
#include "adc.h"
#include "tim.h"
#include "settings.h"

/* External function prototypes ----------------------------------------------*/
extern void SystemClock_Config(void);

/* Private variables ---------------------------------------------------------*/
static uint32_t last_activity_time = 0;

/* Private defines -----------------------------------------------------------*/
#define S_POWER_PIN     GPIO_PIN_9
#define S_POWER_PORT    GPIOC
#define S_ZERO_PIN      GPIO_PIN_2
#define S_ZERO_PORT     GPIOB
#define S_INMM_PIN      GPIO_PIN_4
#define S_INMM_PORT     GPIOA

#define USB_DETECT_THRESHOLD    2.0f    // USB检测阈值 2V
#define BATTERY_DIVIDER         2.0f    // 电池分压比

/**
  * @brief  电源管理初始化
  */
void Power_Init(void)
{
    last_activity_time = HAL_GetTick();
}

/**
  * @brief  开机
  */
void Power_On(void)
{
    last_activity_time = HAL_GetTick();
    
    // 启动传感器电源
    Power_SensorOn();
}

/**
  * @brief  关机
  */
void Power_Off(void)
{
    // 关闭传感器电源
    Power_SensorOff();
    
    // 进入待机模式
    HAL_PWR_EnterSTANDBYMode();
}

/**
  * @brief  进入STOP模式
  */
void Power_EnterStopMode(void)
{
    // 配置唤醒源
    // TODO: 配置定时器唤醒
    
    // 进入STOP模式
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    // 唤醒后恢复时钟
    SystemClock_Config();
}

/**
  * @brief  唤醒
  */
void Power_WakeUp(void)
{
    last_activity_time = HAL_GetTick();
}

/**
  * @brief  传感器上电
  */
void Power_SensorOn(void)
{
    // 拉高S_POWER
    HAL_GPIO_WritePin(S_POWER_PORT, S_POWER_PIN, GPIO_PIN_SET);
    
    // 延迟10ms
    HAL_Delay(10);
    
    // 启动传感器时钟 (TIM5_CH1)
    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
}

/**
  * @brief  传感器下电
  */
void Power_SensorOff(void)
{
    // 停止时钟
    HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_1);
    
    // 延迟10ms
    HAL_Delay(10);
    
    // 拉低S_POWER
    HAL_GPIO_WritePin(S_POWER_PORT, S_POWER_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  检测USB插入
  * @retval 1-USB插入, 0-无USB
  */
uint8_t Power_CheckUSB(void)//当使用USB供电时，设定的关机时间就失效
{
    float voltage = Power_GetBatteryVoltage();
    return (voltage > USB_DETECT_THRESHOLD) ? 1 : 0;
}

/**
  * @brief  获取电池电压
  * @retval 电池电压(V)
  */
float Power_GetBatteryVoltage(void)
{
    uint32_t adc_value;
    float voltage;
    
    // 启动ADC转换
    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, 100);
    adc_value = HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);
    
    // 计算电压 (12位ADC, 3.3V参考电压)
    voltage = (float)adc_value * 3.3f / 4096.0f;
    
    // 考虑分压比
    voltage = voltage * BATTERY_DIVIDER;
    
    return voltage;
}

/**
  * @brief  更新自动关机
  */
void Power_UpdateAutoOff(void)
{
    SystemSettings_t *settings = Settings_Get();
    uint32_t now = HAL_GetTick();
    uint32_t timeout_ms = 0;

    switch (settings->auto_off_time) {
        case AUTO_OFF_15MIN:
            timeout_ms = 15UL * 60UL * 1000UL;
            break;
        case AUTO_OFF_30MIN:
            timeout_ms = 30UL * 60UL * 1000UL;
            break;
        case AUTO_OFF_1H:
            timeout_ms = 60UL * 60UL * 1000UL;
            break;
        case AUTO_OFF_2H:
            timeout_ms = 2UL * 60UL * 60UL * 1000UL;
            break;
        case AUTO_OFF_DISABLE:
        default:
            timeout_ms = 0;
            break;
    }

    if (timeout_ms == 0) {
        return;
    }

    if ((now - last_activity_time) > timeout_ms) {
        if (!Power_CheckUSB()) {
            Power_Off();
        } else {
            last_activity_time = now;
        }
    }
}

/**
  * @brief  传感器清零脉冲
  * @note   发送50ms高电平脉冲到S_ZERO引脚，传感器数据清零
  */
void Power_SensorZero(void)
{
    // 输出50ms高电平脉冲
    HAL_GPIO_WritePin(S_ZERO_PORT, S_ZERO_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(S_ZERO_PORT, S_ZERO_PIN, GPIO_PIN_RESET);
}

/**
  * @brief  传感器单位切换脉冲
  * @note   发送50ms高电平脉冲到S_INMM引脚，切换mm/inch单位
  *         初始为mm，高电平脉冲后转为inch，再次脉冲转回mm
  */
void Power_SensorToggleUnit(void)
{
    // 输出50ms高电平脉冲
    HAL_GPIO_WritePin(S_INMM_PORT, S_INMM_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(S_INMM_PORT, S_INMM_PIN, GPIO_PIN_RESET);
}
