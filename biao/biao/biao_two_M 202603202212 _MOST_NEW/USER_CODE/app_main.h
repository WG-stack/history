/**
  ******************************************************************************
  * @file           : app_main.h
  * @brief          : 应用主程序头文件
  * @author         : 量博科技
  * @date           : 2026-01-07
  ******************************************************************************
  */

#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "sensor_gpio.h"
#include "key_scan.h"
#include "measurement.h"
#include "settings.h"
#include "power_mgmt.h"
#include "lcd_display.h"
#include "battery_monitor.h"
#include "usb_output.h"
#include "uart_bluetooth.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    STATE_POWER_OFF = 0,
    STATE_SELF_TEST,
    STATE_NORMAL,
    STATE_SETTING_MENU,
    STATE_ENGINEERING_MENU,
    STATE_TOL_SETTING,
    STATE_ERROR
} AppState_t;

typedef enum {
    ERR_NONE = 0,
    ERR_NO_DATA,        // Err0: 2秒无数据
    ERR_NO_ORIGIN,      // Err1: 无原位信号
    ERR_DATA_UNSTABLE   // Err2: 数据跳动过大
} ErrorCode_t;

/* Exported functions prototypes ---------------------------------------------*/
void App_Init(void);
void App_Run(void);
void App_SelfTest(void);
void App_ProcessKey(KeyMessage_t *key_msg);
void App_UpdateDisplay(void);
void App_CheckError(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MAIN_H */
