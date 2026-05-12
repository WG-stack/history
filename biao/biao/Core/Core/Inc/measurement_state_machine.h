/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    measurement_state_machine.h
  * @brief   测量状态机头文件
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

#ifndef __MEASUREMENT_STATE_MACHINE_H
#define __MEASUREMENT_STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdint.h"

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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/**
 * @brief 测量模式枚举
 */
typedef enum {
    MEASUREMENT_MODE_NORMAL,
    MEASUREMENT_MODE_ABS,
    MEASUREMENT_MODE_REL,
    MEASUREMENT_MODE_MAX,
    MEASUREMENT_MODE_MIN,
    MEASUREMENT_MODE_TIR,
    MEASUREMENT_MODE_SETUP,
    MEASUREMENT_MODE_ENGINEER_SETUP,
    MEASUREMENT_MODE_TOL_SETTING  // 公差设置模式，用于分别设置TOL_MAX(正公差)和TOL_MIN(负公差)
} MeasurementMode_TypeDef;

/**
 * @brief 公差结果枚举
 */
typedef enum {
    TOLERANCE_RESULT_OK,
    TOLERANCE_RESULT_OVER,
    TOLERANCE_RESULT_UNDER
} ToleranceResult_TypeDef;

/**
 * @brief 分辨率枚举
 */
typedef enum {
    RESOLUTION_0_0005MM,
    RESOLUTION_0_001MM,
    RESOLUTION_0_005MM,
    RESOLUTION_0_01MM
} Resolution_TypeDef;

  /**
   * @brief 测量状态结构体
   */
  typedef struct {
      uint8_t mode;               // 测量模式
      uint8_t unit;               // 测量单位
      bool is_power_on;           // 电源状态 1-开机 0-关机
      int32_t current_value;      // 当前测量值
      int32_t zero_offset;        // 零点偏移量（通用零点偏移，用于非ABS/REL模式）
      int32_t peak_value;         // 峰值测量值（用于MAX/MIN模式）
      int32_t absolute_zero_offset; // 绝对零点偏移量
      bool is_tolerance_enabled;  // 公差使能标志
      uint8_t current_resolution; // 当前分辨率
      int32_t tolerance_upper;    // 上公差值（正公差）
      int32_t tolerance_lower;    // 下公差值（负公差）
      uint8_t tolerance_setting_step;  // 公差设置步骤：0=设置上公差，1=设置下公差
      // 专用基准点，分别用于绝对模式和相对模式
      int32_t abs_bk_data;        // 绝对基准点（ABS_BK_DATA）
      int32_t rer_bk_data;        // 相对基准点（RER_BK_DATA）
      int32_t tol_bk_data;        // 公差模式的基准值（TOL_BK_DATA），用于公差设置模式
      bool is_in_tolerance_check; // 是否处于公差检查状态（附加在ABS/REL模式中）
      int32_t tol_reference_value; // 公差检查的基准值，进入公差检查时的显示值
      uint8_t base_mode;          // 基础模式（REL或ABS），用于在峰值模式和TIR模式之间切换时记住原始模式
 } ZTJ_TypeDef;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */
void ZTJ_Init(void);
void ZTJ_SwitchMode(ZTJ_TypeDef *state);
void ZTJ_SwitchUnit(ZTJ_TypeDef *state);
void ZTJ_ZeroCalibration(ZTJ_TypeDef *state);
uint8_t ZTJ_CheckAutoPowerOff(ZTJ_TypeDef *state, uint32_t idle_time);
void ZTJ_TogglePower(ZTJ_TypeDef *state);
int32_t ZTJ_UpdateMeasureValue(ZTJ_TypeDef *state, int32_t raw_value);
ZTJ_TypeDef* ZTJ_GetCurrentState(void);

// 补充函数声明
void ZTJ_EnterToleranceMode(void);
void ZTJ_CycleMaxMinTir(void);
uint8_t ZTJ_GetCurrentMode(void);
void ZTJ_HandleSetupIncrement(void);
void ZTJ_SwitchAbsToRel(void);
void ZTJ_SwitchRelToAbs(void);
void ZTJ_EnterEngineerSetup(void);
void ZTJ_CycleResolutionMode(void);
void ZTJ_ToggleUnit(void);
void ZTJ_EnterSetupMode(void);
void ZTJ_UpdateLastActivity(void);
ToleranceResult_TypeDef ZTJ_GetToleranceResult(int32_t value);

// 公差设置相关函数声明
void ZTJ_SetToleranceUpper(int32_t upper);
void ZTJ_SetToleranceLower(int32_t lower);
int32_t ZTJ_GetToleranceUpper(void);
int32_t ZTJ_GetToleranceLower(void);
void ZTJ_HandleToleranceSetting(int32_t increment);
void ZTJ_EnterToleranceSettingMode(void);
void ZTJ_ToggleToleranceSettingStep(void);

// TIR模式相关函数声明
int32_t ZTJ_GetTirMaxPositiveDeviation(void);  // 获取TIR最大正跳动
int32_t ZTJ_GetTirMaxNegativeDeviation(void);  // 获取TIR最大负跳动
int32_t ZTJ_GetTirBaseValue(void);  // 获取TIR基准值

// 公差检查附加功能（在ABS/REL模式中切换公差检查状态）
void ZTJ_ToggleToleranceCheck(void);


/* USER CODE END EFP */

#ifdef __cplusplus
}
#endif




#endif /* __MEASUREMENT_STATE_MACHINE_H */
