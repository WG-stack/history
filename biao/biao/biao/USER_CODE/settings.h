/**
  ******************************************************************************
  * @file           : settings.h
  * @brief          : 设置管理模块头文件
  * @author         : 量博科技
  * @date           : 2026-01-07
  ******************************************************************************
  */

#ifndef __SETTINGS_H
#define __SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    DIR_UP_POSITIVE = 0,    // 向上为正
    DIR_DOWN_POSITIVE       // 向下为正
} MoveDirection_t;

typedef enum {
    AUTO_OFF_15MIN = 0,
    AUTO_OFF_30MIN,
    AUTO_OFF_1H,
    AUTO_OFF_2H,
    AUTO_OFF_DISABLE
} AutoOffTime_t;

typedef enum {
    OUTPUT_USB = 0,
    OUTPUT_WIRELESS,
    OUTPUT_NONE
} OutputMode_t;

typedef enum {
    DISPLAY_NORMAL = 0,
    DISPLAY_X2,
    DISPLAY_DIV2,
    DISPLAY_CUSTOM
} DisplayMultiplier_t;

typedef enum {
    SENSOR_TYPE_0_0005MM = 0,
    SENSOR_TYPE_0_001MM
} SensorType_t;

typedef enum {
    AVG_TIMES_0 = 0,
    AVG_TIMES_4,
    AVG_TIMES_8,
    AVG_TIMES_16
} AverageTimes_t;

typedef enum {
    CLK_750KHZ = 0,
    CLK_1MHZ,
    CLK_1_5MHZ,
    CLK_1_75MHZ,
    CLK_2MHZ
} SensorClockFreq_t;

typedef struct {
    // 用户设置
    MoveDirection_t move_direction;
    AutoOffTime_t auto_off_time;
    OutputMode_t output_mode;
    DisplayMultiplier_t display_multiplier;
    float custom_multiplier;
    
    // 工程设置
    SensorType_t sensor_type;
    AverageTimes_t avg_times;
    SensorClockFreq_t sensor_clock;
    
    uint32_t device_id;         // 设备机器码
} SystemSettings_t;

/* Exported functions prototypes ---------------------------------------------*/
void Settings_Init(void);
void Settings_Load(void);
void Settings_Save(void);
void Settings_Reset(void);
SystemSettings_t* Settings_Get(void);

#ifdef __cplusplus
}
#endif

#endif /* __SETTINGS_H */
