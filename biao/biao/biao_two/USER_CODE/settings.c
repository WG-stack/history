/**
  ******************************************************************************
  * @file           : settings.c
  * @brief          : 设置管理模块实现
  ******************************************************************************
  */

#include "settings.h"
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static SystemSettings_t system_settings;

/**
  * @brief  设置初始化
  */
void Settings_Init(void)
{
    // 默认设置
    system_settings.move_direction = DIR_UP_POSITIVE;
    system_settings.auto_off_time = AUTO_OFF_30MIN;
    system_settings.output_mode = OUTPUT_USB;
    system_settings.display_multiplier = DISPLAY_NORMAL;
    system_settings.custom_multiplier = 1.0f;
    
    system_settings.sensor_type = SENSOR_TYPE_0_001MM;
    system_settings.avg_times = AVG_TIMES_8;
    system_settings.sensor_clock = CLK_1_5MHZ;
    
    system_settings.device_id = 0x12345678;  // TODO: 从芯片唯一ID读取
}

/**
  * @brief  从Flash加载设置
  */
void Settings_Load(void)
{
    // TODO: 从Flash读取设置
    // 如果Flash为空或校验失败，使用默认设置
}

/**
  * @brief  保存设置到Flash
  */
void Settings_Save(void)
{
    // TODO: 保存设置到Flash
}

/**
  * @brief  恢复默认设置
  */
void Settings_Reset(void)
{
    Settings_Init();
    Settings_Save();
}

/**
  * @brief  获取设置
  */
SystemSettings_t* Settings_Get(void)
{
    return &system_settings;
}
