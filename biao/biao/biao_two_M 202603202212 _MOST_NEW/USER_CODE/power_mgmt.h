/**
  ******************************************************************************
  * @file           : power_mgmt.h
  * @brief          : 电源管理模块头文件
  * @author         : 量博科技
  * @date           : 2026-01-07
  ******************************************************************************
  */

#ifndef __POWER_MGMT_H
#define __POWER_MGMT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    POWER_ON = 0,
    POWER_STANDBY,
    POWER_OFF
} PowerState_t;

/* Exported functions prototypes ---------------------------------------------*/
void Power_Init(void);
void Power_On(void);
void Power_Off(void);
void Power_EnterStopMode(void);
void Power_WakeUp(void);
void Power_SensorOn(void);
void Power_SensorOff(void);
void Power_SensorZero(void);
void Power_SensorToggleUnit(void);
uint8_t Power_CheckUSB(void);
float Power_GetBatteryVoltage(void);
void Power_UpdateAutoOff(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_MGMT_H */
