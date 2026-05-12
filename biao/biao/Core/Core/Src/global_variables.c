/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : global_variables.c
  * @brief          : Global variables implementation file for structured programming
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
/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : global_variables.c
  * @brief          : Global variables implementation file
  ******************************************************************************
  */
/* USER CODE END Header */
#include "global_variables.h"
#include "key_driver.h"

/* Global variables for system state */
uint32_t last_key_time = 0;
uint32_t key_response_end_time = 0;
bool is_in_key_response = false;
volatile uint8_t key_scan_pending = 0;  // 按键扫描标志

/* Global variables for sensor data processing */
SensorData_TypeDef averaged_sensor_value = {false}; // 平均传感器值结构体，初始is_valid为false
SensorData_TypeDef processed_data = {false}; // 初始化结构体，is_valid为false
SensorData_TypeDef result_data = {false};    // 全局结果数据结构体，初始is_valid为false

/* Global variables for key handling */
 KeyScanResult_TypeDef key_result = {KEY_NONE, KEY_EVENT_NONE, false};

/* Global state machine variables */
SystemState_t system_state = STATE_INIT;
KeyStateMachine_t key_fsm = {.value = 0};
KeyEventFlags_t key_event_flags = {.value = 0};

/* Global variables for main loop control */
volatile Sample_sample_t current_sample = {.value = 0};
uint8_t current_loop_iteration = 0;

/* Measurement state machine */
ZTJ_TypeDef measurement_state = {0}; // 初始化状态机参数

/* LCD Display state machine - 移除全局定义，避免头文件循环依赖 */
// LCD_DisplayState_TypeDef lcd_display_state = {0}; // 初始化LCD显示状态机参数

/* Global variables for sensor interrupt handling */
volatile bool sensor_data_ready_flag = false;

/* Debug flag for interrupt monitoring */
volatile bool interrupt_debug_flag = false;

/* Correction factor for sensor readings */
float xiuzhengXS = 1.0f;  // 修正系数，默认值为1.0

/* Global variable for manual zero calibration */
uint8_t manual_zero_calibration_requested = 0;

/* Global variable for sensor type - 0.5u=1, 1u=2 */
SensorType_TypeDef sensor_type = SENSOR_TYPE_UNKNOWN;

/* 公差设置模式相关全局变量 */
uint8_t LOOP_tol_LCD = 0;  // 用于公差设置模式下图标闪烁的主循环计数器

/* 模拟指针相关全局变量 */
int8_t analog_pointer_value = 0;  // 模拟指针值 (-15 到 +15)

/* TIM7 interrupt counter */
uint32_t tim7_interrupt_count = 0;  // 添加TIM7中断计数器定义

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */