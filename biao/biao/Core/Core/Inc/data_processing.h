/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    data_processing.h
  * @brief   数据处理机头文件
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __DATA_PROCESSING_H
#define __DATA_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "measurement_state_machine.h"
#include "global_variables.h"  // 包含SensorData_TypeDef的定义

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* SensorData_TypeDef is now defined in global_variables.h */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern int32_t Rer_data;  // 相对值数据
extern int32_t Abs_data;  // 绝对值数据
extern int32_t Rew_data;  // 滤波后的数据
extern int32_t Rer_bk_data;  // 相对基准点
extern int32_t Abs_bk_data;  // 绝对基准点
extern int8_t analog_pointer_value;  // 模拟指针值
extern SensorData_TypeDef averaged_sensor_value;  // 平均传感器值
extern SensorData_TypeDef result_data;  // 结果数据
extern float xiuzhengXS;  // 修正系数
extern uint8_t manual_zero_calibration_requested;  // 手动零点校准请求标志
extern uint8_t sensor_type;  // 传感器类型
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */
void DataProcessing_Init(void);
SensorData_TypeDef DataProcessing_Process(int32_t raw_value);
void Data_Filter_Reset(int32_t reset_value);

// 用户数据处理流程相关函数
void DataProcessing_SetAbsBaseline(int32_t baseline_value);
void DataProcessing_SetRelBaselineFromAbs(void);
int32_t DataProcessing_GetAbsData(void);

/* USER CODE END EFP */

/* 模拟指针处理函数声明 */
void DataProcessing_UpdateAnalogPointer(int32_t input_value, Resolution_TypeDef resolution);
int8_t DataProcessing_CalculateAnalogPointerValue(int32_t input_value, Resolution_TypeDef resolution);

#ifdef __cplusplus
}
#endif

#endif /* __DATA_PROCESSING_H */
