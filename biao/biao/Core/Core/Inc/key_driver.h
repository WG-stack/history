/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    key_driver.h
  * @brief   按键驱动模块头文件
  * @author  AI
  ******************************************************************************
  * @attention
  *
  * 该文件包含按键驱动的定义和接口
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __KEY_DRIVER_H__
#define __KEY_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* Define boolean type for systems that don't support stdbool.h */
#ifndef __cplusplus
#ifndef bool
typedef unsigned char bool;
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

/* 按键定义 */
typedef enum {
    KEY_NONE = 0,           // 无按键
    KEY_ON_ZERO,            // ON/Zero 键
    KEY_DOWN_ABS_SET,       // DOWN/ABS/SET 键
    KEY_UP_IN_MM,           // UP/IN/MM 键
    KEY_TOL,                // TOL 键
    KEY_M,                  // M 键
    KEY_ABS_ZERO_SWITCH     // ABS_ZERO 原位开关
} Key_TypeDef;

/* 按键长按时间阈值（毫秒）*/
#define KEY_LONG_PRESS_TIME_MS    1000

/* 按键状态定义 */
typedef enum {
    KEY_STATUS_RELEASED = 0,    // 按键释放
    KEY_STATUS_PRESSED,         // 按键按下
    KEY_STATUS_LONG_PRESS       // 按键长按
} KeyStatus_TypeDef;

/* 按键事件定义 */
typedef enum {
    KEY_EVENT_NONE = 0,         // 无事件
    KEY_EVENT_SHORT_PRESS,      // 短按事件
    KEY_EVENT_LONG_PRESS,       // 长按事件
    KEY_EVENT_RELEASE,          // 按键释放事件
    KEY_EVENT_LONG_PRESS_RELEASE // 长按释放事件
} KeyEvent_TypeDef;

/* 按键扫描结果结构体 */
typedef struct {
    Key_TypeDef key_type;       // 按键类型
    KeyEvent_TypeDef event;     // 按键事件
    bool is_pressed;            // 是否按下标志
} KeyScanResult_TypeDef;

/* 按键驱动初始化 */
void KeyDriver_Init(void);

/* 按键扫描函数 */
KeyScanResult_TypeDef KeyDriver_Scan(void);

/* 按键处理函数 */
void KeyDriver_Process(KeyScanResult_TypeDef *result);

/* 获取按键状态 */
uint8_t KeyDriver_GetStatus(Key_TypeDef key);

/* 按键矩阵扫描 */
Key_TypeDef KeyDriver_MatrixScan(void);

/* 检查是否有任意按键被按下 */
uint8_t KeyDriver_IsAnyKeyPressed(void);

/* 定时器相关函数 - 集成到KEY模块内 */
bool KeyDriver_Timer_Init(void);           // 定时器初始化
void KeyDriver_Timer_Start(void);          // 启动定时器
void KeyDriver_Timer_Stop(void);           // 停止定时器
void KeyDriver_Timer_Scan(void);           // 定时器扫描函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim); // 定时器中断回调

#ifdef __cplusplus
}
#endif

#endif /* __KEY_DRIVER_H__ */