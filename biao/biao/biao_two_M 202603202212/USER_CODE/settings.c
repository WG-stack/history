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

#define SETTINGS_MAGIC_NUMBER  0x5AA55AA5
#define SETTINGS_EEPROM_ADDR   (0x08080000UL) // DATA EEPROM base address for STM32L1

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
    
    system_settings.sensor_type = SENSOR_TYPE_0_0005MM;
    system_settings.avg_times = AVG_TIMES_8;
    system_settings.sensor_clock = CLK_1_5MHZ;
    
    system_settings.device_id = 0x12345678;  // TODO: 从芯片唯一ID读取
}

/**
  * @brief  从Flash加载设置
  */
void Settings_Load(void)
{
    uint32_t magic = *(__IO uint32_t*)SETTINGS_EEPROM_ADDR;
    if (magic == SETTINGS_MAGIC_NUMBER) {
        // 魔数匹配，读取设置
        uint32_t *dest = (uint32_t*)&system_settings;
        uint32_t *src = (uint32_t*)(SETTINGS_EEPROM_ADDR + 4);
        uint32_t words = (sizeof(SystemSettings_t) + 3) / 4;
        
        for (uint32_t i = 0; i < words; i++) {
            dest[i] = src[i];
        }
    } else {
        // 如果Flash为空或校验失败，使用默认设置并保存
        Settings_Init();
        Settings_Save();
    }
}

/**
  * @brief  保存设置到Flash
  */
void Settings_Save(void)
{
    uint32_t *src = (uint32_t*)&system_settings;
    uint32_t words = (sizeof(SystemSettings_t) + 3) / 4;
    
    // 解锁DATA EEPROM
    HAL_FLASHEx_DATAEEPROM_Unlock();
    
    // 写入魔数
    HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, SETTINGS_EEPROM_ADDR, SETTINGS_MAGIC_NUMBER);
    
    // 写入结构体数据
    for (uint32_t i = 0; i < words; i++) {
        HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, SETTINGS_EEPROM_ADDR + 4 + (i * 4), src[i]);
    }
    
    // 锁定DATA EEPROM
    HAL_FLASHEx_DATAEEPROM_Lock();
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
