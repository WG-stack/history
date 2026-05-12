/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : global_variables.h
  * @brief          : Global variables header file for structured programming
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __GLOBAL_VARIABLES_H
#define __GLOBAL_VARIABLES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "key_driver.h"  // 添加按键驱动头文件以获取KeyScanResult_TypeDef定义
#include "measurement_state_machine.h"  // 添加测量状态机头文件

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

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Global variables for system state - 全局变量定义，便于查看和管理 */
extern uint32_t last_key_time;                    // 记录最后一次按键的时间
extern uint32_t key_response_end_time;            // 按键响应显示结束时间
extern bool is_in_key_response;                   // 标记是否正在显示按键响应结果
extern volatile uint8_t key_scan_pending;         // 按键扫描标志


/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* 传感器类型枚举 */
typedef enum {
    SENSOR_TYPE_UNKNOWN = 0,
    SENSOR_TYPE_0_5UM,        // 0.5um传感器
    SENSOR_TYPE_1UM,          // 1um传感器
    SENSOR_TYPE_END
} SensorType_TypeDef;

/* Sensor data structure definition moved from data_processing.h */
typedef struct {
    bool is_valid;
    int32_t value;
    uint8_t sign;
    uint8_t unit;
    uint8_t mode;
    uint8_t digits[6];      // 新增：6位数字数组
    uint8_t unit_metric;    // 新增：1=mm 0=in
    uint8_t last_digit;     // 新增：最后一位小数
} SensorData_TypeDef;

/* USER CODE END ET */

/* Global variables for sensor data processing */
extern SensorData_TypeDef averaged_sensor_value;
extern SensorData_TypeDef processed_data;
extern SensorData_TypeDef result_data;

/* Global variable for sensor type - 0.5u=1, 1u=2 */
extern SensorType_TypeDef sensor_type;

/* Global variables for key handling */
extern KeyScanResult_TypeDef key_result;

/* Global variables for sensor interrupt handling */
extern volatile bool sensor_data_ready_flag;

/* Debug flag for interrupt monitoring */
extern volatile bool interrupt_debug_flag;

/* Flag for manual zero calibration - indicates that the next processed data should be used as baseline */
extern uint8_t manual_zero_calibration_requested;

/* Correction factor for sensor readings */
extern float xiuzhengXS;  // 修正系数

/* Global state machine variables - 全局状态机定义 */
typedef enum {
    STATE_INIT = 0,
    STATE_SAMPLING,
    STATE_PROCESSING,
    STATE_KEY_HANDLING,
    STATE_DISPLAY_UPDATE,
    STATE_DELAY
} SystemState_t;

extern SystemState_t system_state;

/* Measurement state machine - 智能测量状态机相关全局变量 */
extern ZTJ_TypeDef measurement_state;

/* LCD Display state machine - 声明在lcd_display.h中定义的类型 */
// extern LCD_DisplayState_TypeDef lcd_display_state;  // 暂时注释掉，避免头文件循环依赖

/* Key state machine related global variables - 按键状态机相关全局变量 */
typedef union {
    uint32_t value;                                    // 整个状态值
    struct {
        uint8_t s_tol : 1;      // 短按键的TOL值 (0或1)
        uint8_t s_m : 1;        // 短按键的M值 (0或1)
        uint8_t s_up_in_mm : 1; // 短按键的UP IN MM值 (0或1)
        uint8_t s_down_abs_set : 1; // 短按键的DOWN ABS SET值 (0或1)
        uint8_t s_on_zero : 1;  // 短按键的ON ZERO值 (0或1)
        uint8_t s_abszero : 1;  // 短按键的ABS ZERO值 (0或1)
        uint8_t l_tol : 1;      // 长按键的TOL值 (0或1)
        uint8_t l_m : 1;        // 长按键的M值 (0或1)
        uint8_t l_up_in_mm : 1; // 长按键的UP IN MM值 (0或1)
        uint8_t l_down_abs_set : 1; // 长按键的DOWN ABS SET值 (0或1)
        uint8_t l_on_zero : 1;  // 长按键的ON ZERO值 (0或1)
        uint8_t l_abszero : 1;  // 长按键的ABS ZERO值 (0或1)
        uint8_t reserved : 20; // 保留位
    } bits;
} KeyStateMachine_t;

extern KeyStateMachine_t key_fsm;

/* Key event flags - 按键事件标志，用于状态机处理 */
typedef union {
    uint32_t value;                                    // 整个事件标志值
    struct {
        uint8_t s_tol_evt : 1;      // 短按TOL事件标志
        uint8_t s_m_evt : 1;        // 短按M事件标志
        uint8_t s_up_in_mm_evt : 1; // 短按UP/IN/MM事件标志
        uint8_t s_down_abs_set_evt : 1; // 短按DOWN/ABS/SET事件标志
        uint8_t s_on_zero_evt : 1;  // 短按ON/ZERO事件标志
        uint8_t s_abszero_evt : 1;  // 短按ABS ZERO事件标志
        uint8_t l_tol_evt : 1;      // 长按TOL事件标志
        uint8_t l_m_evt : 1;        // 长按M事件标志
        uint8_t l_up_in_mm_evt : 1; // 长按UP/IN/MM事件标志
        uint8_t l_down_abs_set_evt : 1; // 长按DOWN/ABS/SET事件标志
        uint8_t l_on_zero_evt : 1;  // 长按ON/ZERO事件标志
        uint8_t l_abszero_evt : 1;  // 长按ABS ZERO事件标志
        uint8_t reserved : 20;      // 保留位
    } bits;
} KeyEventFlags_t;

extern KeyEventFlags_t key_event_flags;

/* Sensor sample state machine related global variables - 传感器采样状态机相关全局变量 */
//
typedef union {
    uint32_t value;  // 低24位有效
    struct {
        uint32_t sample_data : 20;
        uint8_t  sample_x1 : 1;      // 无效位X1
        uint8_t  sample_sign : 1;    // SIGN位（正负号位）
        uint8_t  sample_x2 : 1;
        uint8_t  sample_unit : 1;
    } bits;

} Sample_sample_t;

extern volatile Sample_sample_t current_sample;
extern uint8_t current_loop_iteration;

/* 公差设置模式相关全局变量 */
extern uint8_t LOOP_tol_LCD;  // 用于公差设置模式下图标闪烁的主循环计数器

/* 模拟指针相关全局变量 */
extern int8_t analog_pointer_value;  // 模拟指针值 (-15 到 +15)

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif


#endif /* __GLOBAL_VARIABLES_H */
