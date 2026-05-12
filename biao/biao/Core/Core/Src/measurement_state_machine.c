/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    measurement_state_machine.c
  * @brief   测量状态机驱动文件
  ******************************************************************************
  * @attention
  *
  * 工作流程：
  * 1. 初始化测量状态机，设置默认模式、单位、分辨率等参数
  * 2. 根据用户按键输入切换测量模式（NORMAL/ABS/REL/MAX/MIN/TIR/TOL等）
  * 3. 处理单位切换（MM/IN）、零点校准、公差设置等功能
  * 4. 根据当前模式处理传感器输入值（如峰值检测、公差判断等）
  * 5. 提供状态查询接口供其他模块使用
  *
  * 实现函数：
  * - ZTJ_Init(): 测量状态机初始化函数
  * - ZTJ_SwitchMode(): 切换测量模式函数
  * - ZTJ_SwitchUnit(): 切换测量单位函数
  * - ZTJ_ZeroCalibration(): 执行零点校准函数
  * - ZTJ_CheckAutoPowerOff(): 检查自动关机条件函数
  * - ZTJ_TogglePower(): 切换电源状态函数
  * - ZTJ_UpdateMeasureValue(): 更新测量数值函数
  * - ZTJ_GetCurrentState(): 获取当前测量状态函数
  * - ZTJ_ToggleToleranceCheck(): 切换公差检查状态函数（附加在ABS/REL模式中）
  * - ZTJ_CycleMaxMinTir(): 循环切换MAX/MIN/TIR模式函数
  * - ZTJ_GetCurrentMode(): 获取当前测量模式函数
  * - ZTJ_HandleSetupIncrement(): 处理设置模式下的增量操作函数
  * - ZTJ_SwitchAbsToRel(): 从绝对模式切换到相对模式函数
  * - ZTJ_SwitchRelToAbs(): 从相对模式切换到绝对模式函数
  * - ZTJ_EnterEngineerSetup(): 进入工程师设置模式函数
  * - ZTJ_CycleResolutionMode(): 循环切换分辨率模式函数
  * - ZTJ_ToggleUnit(): 快速切换测量单位函数
  * - ZTJ_EnterSetupMode(): 进入用户设置模式函数
  * - ZTJ_UpdateLastActivity(): 更新最后操作时间戳函数
  * - ZTJ_GetToleranceResult(): 获取公差判断结果函数
  * - ZTJ_GetTirMaxPositiveDeviation(): 获取TIR模式最大正跳动函数
  * - ZTJ_GetTirMaxNegativeDeviation(): 获取TIR模式最大负跳动函数
  * - ZTJ_GetTirBaseValue(): 获取TIR模式基准值函数
  * - ZTJ_EnterToleranceSettingMode(): 进入公差值设置模式函数
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

/* Includes ------------------------------------------------------------------*/
#include "measurement_state_machine.h"
#include "main.h"
#include "stm32l1xx_hal.h"
#include <stdlib.h>
#include "global_variables.h"  // 包含SensorData_TypeDef和相关全局变量定义
#include "data_processing.h"   // 包含数据处理函数定义
#include "lcd_display.h"       // 包含LCD显示函数声明

// 全局变量：记录最后一次活动时间
uint32_t last_activity_time = 0;

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/**
 * @brief 测量单位枚举
 */
typedef enum {
    UNIT_MM = 0,                // 毫米单位
    UNIT_IN,                    // 英寸单位
    UNIT_MAX
} MeasureUnit_TypeDef;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AUTO_POWER_OFF_TIME     300000  // 自动关机时间 5分钟(ms)
#define UNIT_CONVERT_RATIO      25.4f   // 英寸转毫米系数 1in=25.4mm
#define ZERO_CALIBRATION_VALUE  0       // 零点校准基准值
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// 测量状态结构体实例由extern ZTJ_TypeDef measurement_state;声明和使用

static int32_t tir_base_value = 0;  // TIR模式基准值
static int32_t max_deviation_positive = 0;  // TIR最大正跳动
static int32_t max_deviation_negative = 0;  // TIR最大负跳动

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static int32_t Unit_Convert(int32_t value, MeasureUnit_TypeDef from_unit, MeasureUnit_TypeDef to_unit);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  测量单位转换函数
 * @param  value 待转换数值
 * @param  from_unit 原单位
 * @param  to_unit 目标单位
 * @retval 转换后数值
 */
static int32_t Unit_Convert(int32_t value, MeasureUnit_TypeDef from_unit, MeasureUnit_TypeDef to_unit)
{
    if(from_unit == to_unit) {
        return value;
    }
    
    float temp_value = (float)value;
    
    if(from_unit == UNIT_MM && to_unit == UNIT_IN) {
        temp_value /= UNIT_CONVERT_RATIO;
    } else if(from_unit == UNIT_IN && to_unit == UNIT_MM) {
        temp_value *= UNIT_CONVERT_RATIO;
    }
    
    return (int32_t)temp_value;
}

/* USER CODE END 0 */

/* Exported functions --------------------------------------------------------*/
/* USER CODE BEGIN EF */

/**
 * @brief 初始化测量状态机
 * @param  None
 * @retval None
 */
void ZTJ_Init(void)
{
    extern ZTJ_TypeDef measurement_state;
	measurement_state.mode = 1;//MEASUREMENT_MODE_NORMAL;
    measurement_state.unit = UNIT_MM;
    measurement_state.is_power_on = true;
    measurement_state.current_value = 0;
    measurement_state.zero_offset = ZERO_CALIBRATION_VALUE;
    measurement_state.peak_value = 0;
    measurement_state.absolute_zero_offset = 0;
    measurement_state.is_tolerance_enabled = false;
    measurement_state.current_resolution = RESOLUTION_0_001MM;
    measurement_state.tolerance_upper = 0;    // 上公差值初始化
    measurement_state.tolerance_lower = 0;    // 下公差值初始化
    measurement_state.tolerance_setting_step = 0;  // 公差设置步骤初始化
    // 初始化基准点字段
    measurement_state.abs_bk_data = 0;
    measurement_state.rer_bk_data = 0;
    measurement_state.tol_bk_data = 0;        // 初始化公差基准值
    measurement_state.is_in_tolerance_check = false;  // 初始化不在公差检查状态
    measurement_state.tol_reference_value = 0;        // 初始化公差参考值
    measurement_state.base_mode = MEASUREMENT_MODE_REL;  // 初始化基础模式为REL模式
    last_activity_time = HAL_GetTick();
}

/**
 * @brief  切换测量模式
 * @param  state 测量状态结构体指针
 * @retval None
 */
void ZTJ_SwitchMode(ZTJ_TypeDef *state)
{
    extern ZTJ_TypeDef measurement_state;
    if(state == NULL) {
        state = &measurement_state;
    }
    
    state->mode++;
    if(state->mode >= MEASUREMENT_MODE_TOL_SETTING + 1) {
        state->mode = MEASUREMENT_MODE_NORMAL;
    }
    
    // 切换模式后重置峰值
    if(state->mode == MEASUREMENT_MODE_MAX) {
        // 使用相对数据作为基准值
        extern int32_t Rer_data;  // 引用data_processing.c中的相对数据
        state->peak_value = Rer_data;
    } else if(state->mode == MEASUREMENT_MODE_MIN) {
        // 使用相对数据作为基准值
        extern int32_t Rer_data;  // 引用data_processing.c中的相对数据
        state->peak_value = Rer_data;
    }
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  切换测量单位
 * @param  state 测量状态结构体指针
 * @retval None
 */
void ZTJ_SwitchUnit(ZTJ_TypeDef *state)
{
    extern ZTJ_TypeDef measurement_state;
    if(state == NULL) {
        state = &measurement_state;
    }
    
    state->unit++;
    if(state->unit >= UNIT_MAX) {
        state->unit = UNIT_MM;
    }
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  执行零点校准
 * @param  state 测量状态结构体指针
 * @retval None
 */
void ZTJ_ZeroCalibration(ZTJ_TypeDef *state)
{
    extern ZTJ_TypeDef measurement_state;
    if(state == NULL) {
        state = &measurement_state;
    }
    
    // 记录当前零点偏移量
    state->zero_offset = state->current_value;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  检查是否需要自动关机
 * @param  state 测量状态结构体指针
 * @param  idle_time 空闲时间(ms)
 * @retval 1-需要关机 0-不需要关机
 */
uint8_t ZTJ_CheckAutoPowerOff(ZTJ_TypeDef *state, uint32_t idle_time)
{
    extern ZTJ_TypeDef measurement_state;
    if(state == NULL) {
        state = &measurement_state;
    }
    
    if(!state->is_power_on) {
        return 0;
    }
    
    if(idle_time >= AUTO_POWER_OFF_TIME) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief  切换电源状态
 * @param  state 测量状态结构体指针
 * @retval None
 */
void ZTJ_TogglePower(ZTJ_TypeDef *state)
{
    extern ZTJ_TypeDef measurement_state;
    if(state == NULL) {
        state = &measurement_state;
    }
    
    state->is_power_on = !state->is_power_on;
    
    if(state->is_power_on) {
        // 开机初始化
        ZTJ_Init();
    } else {
        // 关机前清空状态
        state->current_value = 0;
        state->peak_value = 0;
    }
}

/**
 * @brief  更新测量数值
 * @param  state 测量状态结构体指针
 * @param  raw_value 原始测量值
 * @retval 处理后的最终测量值
 */
int32_t ZTJ_UpdateMeasureValue(ZTJ_TypeDef *state, int32_t raw_value)
{
    extern ZTJ_TypeDef measurement_state;
    if(state == NULL) {
        state = &measurement_state;
    }
    
    int32_t cali_value;
    // 在绝对模式和正常模式下直接使用raw_value（即Abs_data）
    if(state->mode == MEASUREMENT_MODE_ABS || state->mode == MEASUREMENT_MODE_NORMAL) {
        // 在绝对模式和正常模式下，raw_value已经是经过Abs_data = Rew_data - Abs_bk_data计算后的值
        // 所以直接使用raw_value即可，不需要再减去基准
        cali_value = raw_value;
    } else if(state->mode == MEASUREMENT_MODE_REL) {
        // 在相对模式下，使用相对基准点
        cali_value = raw_value;  // Rer_data已经是处理过的相对值
    } else {
        // 在其他模式下使用通用零点偏移
        cali_value = raw_value - state->zero_offset;
    }
    
    // 根据模式处理数值
    switch(state->mode) {
        case MEASUREMENT_MODE_MAX:
            if(cali_value > state->peak_value) {
                state->peak_value = cali_value;
            }
            state->current_value = state->peak_value;
            break;
        case MEASUREMENT_MODE_MIN:
            if(cali_value < state->peak_value) {
                state->peak_value = cali_value;
            }
            state->current_value = state->peak_value;
            break;
        case MEASUREMENT_MODE_TIR:
            // TIR模式：数字显示部分和NORMAL模式一样（显示当前值），模拟指针显示最大正跳动和最大负跳动
            // 使用全局变量来跟踪TIR模式的状态
            
            // 检查是否刚进入TIR模式（通过检查tir_base_value是否为初始值）
            if(tir_base_value == 0 && max_deviation_positive == 0 && max_deviation_negative == 0) {
                // 第一次进入TIR模式，设置基准值为相对数据
                extern int32_t Rer_data;  // 引用data_processing.c中的相对数据
                tir_base_value = Rer_data;
                max_deviation_positive = 0;
                max_deviation_negative = 0;
            }
            
            // 计算相对于基准值的偏差
            int32_t deviation = 0; // Initialize to avoid warning
            deviation = cali_value - tir_base_value;
            
            // 更新最大正跳动和最大负跳动
            if(deviation > max_deviation_positive) {
                max_deviation_positive = deviation;
            }
            if(deviation < max_deviation_negative) {
                max_deviation_negative = deviation;
            }
            
            // 数字显示部分和NORMAL模式一样，显示当前测量值（经过校准的值）
            state->current_value = cali_value;
            break;
            
            
        default:
            state->current_value = cali_value;
            break;
    }
    
    // 单位转换
    return Unit_Convert(state->current_value, UNIT_MM, state->unit);
}

/**
 * @brief  获取当前测量状态
 * @param  None
 * @retval 测量状态结构体指针
 */
ZTJ_TypeDef* ZTJ_GetCurrentState(void)
{
    extern ZTJ_TypeDef measurement_state;
    return &measurement_state;
}


/**
 * @brief  循环切换 MAX/MIN/TIR/退出 模式
 * @param  None
 * @retval None
 */
void ZTJ_CycleMaxMinTir(void)
{
    extern ZTJ_TypeDef measurement_state;
    switch(measurement_state.mode) {
        case MEASUREMENT_MODE_REL:
            measurement_state.mode = MEASUREMENT_MODE_MAX;
            measurement_state.base_mode = MEASUREMENT_MODE_REL;  // 记录当前的基础模式
            // 进入MAX模式时，将peak_value设置为相对数据作为基准
            extern int32_t Rer_data;  // 引用data_processing.c中的相对数据
            measurement_state.peak_value = Rer_data;
            break;
        case MEASUREMENT_MODE_ABS:
            measurement_state.mode = MEASUREMENT_MODE_MAX;
            measurement_state.base_mode = MEASUREMENT_MODE_ABS;  // 记录当前的基础模式
            // 进入MAX模式时，将peak_value设置为绝对数据作为基准
            extern int32_t Abs_data;  // 引用data_processing.c中的绝对数据
            measurement_state.peak_value = Abs_data;
            break;
        case MEASUREMENT_MODE_MAX:
            measurement_state.mode = MEASUREMENT_MODE_MIN;
            // 进入MIN模式时，peak_value保持不变，继续使用MAX模式设置的基准值
            // 不需要重新设置peak_value，保持与MAX模式相同的基准值
            break;
        case MEASUREMENT_MODE_MIN:
            measurement_state.mode = MEASUREMENT_MODE_TIR;
            // 进入TIR模式时，使用与MAX/MIN相同的基准值
            // 使用当前的peak_value作为TIR的基准值，确保三者使用相同的基准
            tir_base_value = measurement_state.peak_value;
            max_deviation_positive = 0;
            max_deviation_negative = 0;
            break;
        case MEASUREMENT_MODE_TIR:
            // 从TIR模式出来时，返回到原来的基础模式（REL或ABS）
            if(measurement_state.base_mode == MEASUREMENT_MODE_ABS) {
                measurement_state.mode = MEASUREMENT_MODE_ABS;
            } else {
                measurement_state.mode = MEASUREMENT_MODE_REL;
            }
            // 重置TIR相关变量
            tir_base_value = 0;
            max_deviation_positive = 0;
            max_deviation_negative = 0;
            break;
        default:
            // 如果当前不是REL或ABS模式，则默认切换到REL模式再进入MAX
            measurement_state.mode = MEASUREMENT_MODE_REL;
            measurement_state.base_mode = MEASUREMENT_MODE_REL;  // 记录当前的基础模式
            break;
    }
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  获取当前测量模式
 * @param  None
 * @retval 当前测量模式枚举值
 */
uint8_t ZTJ_GetCurrentMode(void)
{
    extern ZTJ_TypeDef measurement_state;
    return measurement_state.mode;
}

/**
 * @brief  处理设置模式下的增量操作
 * @param  None
 * @retval None
 */
void ZTJ_HandleSetupIncrement(void)
{
    // 可根据实际需求添加参数调整逻辑
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  从绝对模式切换到相对模式
 * @param  None
 * @retval None
 */
void ZTJ_SwitchAbsToRel(void)
{
    extern ZTJ_TypeDef measurement_state;
    // 从数据处理模块获取当前的绝对值作为相对基准点
    extern int32_t DataProcessing_GetAbsData(void);
    int32_t current_abs_value = DataProcessing_GetAbsData();
    
    measurement_state.mode = MEASUREMENT_MODE_REL;
    // 设置相对基准点为当前绝对值，实现进入相对模式时的"清零"效果
    measurement_state.rer_bk_data = current_abs_value;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  从相对模式切换到绝对模式
 * @param  None
 * @retval None
 */
void ZTJ_SwitchRelToAbs(void)
{
    extern ZTJ_TypeDef measurement_state;
    
    measurement_state.mode = MEASUREMENT_MODE_ABS;
    // 切换到绝对模式时，使用相对基准点作为新的绝对基准
    measurement_state.abs_bk_data = measurement_state.rer_bk_data;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  进入工程师设置模式
 * @param  None
 * @retval None
 */
void ZTJ_EnterEngineerSetup(void)
{
    extern ZTJ_TypeDef measurement_state;
    measurement_state.mode = MEASUREMENT_MODE_ENGINEER_SETUP;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  循环切换分辨率模式
 * @param  None
 * @retval None
 */
void ZTJ_CycleResolutionMode(void)
{
    extern ZTJ_TypeDef measurement_state;
    
    Resolution_TypeDef old_resolution = (Resolution_TypeDef)measurement_state.current_resolution;
    measurement_state.current_resolution++;
    if((Resolution_TypeDef)measurement_state.current_resolution > RESOLUTION_0_01MM) {
        measurement_state.current_resolution = RESOLUTION_0_0005MM;
    }
    
    Resolution_TypeDef new_resolution = (Resolution_TypeDef)measurement_state.current_resolution;
    
    // 只有当分辨率真正改变时才记录活动时间
    if(old_resolution != new_resolution) {
        ZTJ_UpdateLastActivity();
    }
}

/**
 * @brief  快速切换测量单位（兼容旧接口）
 * @param  None
 * @retval None
 */
void ZTJ_ToggleUnit(void)
{
    ZTJ_SwitchUnit(NULL);
}

/**
 * @brief  进入用户设置模式
 * @param  None
 * @retval None
 */
void ZTJ_EnterSetupMode(void)
{
    extern ZTJ_TypeDef measurement_state;
    measurement_state.mode = MEASUREMENT_MODE_SETUP;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief  更新最后操作时间戳
 * @param  None
 * @retval None
 */
void ZTJ_UpdateLastActivity(void)
{
    last_activity_time = HAL_GetTick();
}

/**
* @brief 获取公差判断结果
* @param  value 当前测量值
* @retval 公差结果枚举值
*/
ToleranceResult_TypeDef ZTJ_GetToleranceResult(int32_t value)
{
  extern ZTJ_TypeDef measurement_state;
  // 如果公差功能未启用或不在公差检查状态，返回OK
  if (!measurement_state.is_tolerance_enabled || !measurement_state.is_in_tolerance_check) {
      return TOLERANCE_RESULT_OK;
  }

  // 实现公差判断逻辑：
  // 检查当前测量值是否在[进入公差检查时的基准值 + 下公差, 进入公差检查时的基准值 + 上公差]范围内
  // 即：如果 基准值 + 下公差 <= 当前值 <= 基准值 + 上公差，则判断为OK
  // 否则判断为NG（超出公差范围）
  // 注意：tolerance_upper是正公差，tolerance_lower是负公差（通常为负值）
  int32_t reference_value = measurement_state.tol_reference_value;  // 进入公差检查时的基准值
  int32_t upper_limit = reference_value + measurement_state.tolerance_upper;  // 上限：基准值 + 上公差
  int32_t lower_limit = reference_value + measurement_state.tolerance_lower;  // 下限：基准值 + 下公差

  // 检查当前值是否在公差范围内
  if (value >= lower_limit && value <= upper_limit) {
      return TOLERANCE_RESULT_OK;      // 在公差范围内，显示OK
  } else {
      return TOLERANCE_RESULT_OVER;    // 超出公差范围，显示NG
  }
}

/**
 * @brief 设置上公差值（正公差）
 * @param  upper 上公差值
 * @retval None
 */
void ZTJ_SetToleranceUpper(int32_t upper)
{
    extern ZTJ_TypeDef measurement_state;
    measurement_state.tolerance_upper = upper;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief 设置下公差值（负公差）
 * @param  lower 下公差值
 * @retval None
 */
void ZTJ_SetToleranceLower(int32_t lower)
{
    extern ZTJ_TypeDef measurement_state;
    measurement_state.tolerance_lower = lower;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief 获取上公差值
 * @param  None
 * @retval 上公差值
 */
int32_t ZTJ_GetToleranceUpper(void)
{
    extern ZTJ_TypeDef measurement_state;
    return measurement_state.tolerance_upper;
}

/**
 * @brief 获取下公差值
 * @param  None
 * @retval 下公差值
 */
int32_t ZTJ_GetToleranceLower(void)
{
    extern ZTJ_TypeDef measurement_state;
    return measurement_state.tolerance_lower;
}

/**
 * @brief 处理公差设置模式下的值调整
 * @param  increment 调整步进值
 * @retval None
 */
void ZTJ_HandleToleranceSetting(int32_t increment)
{
     extern ZTJ_TypeDef measurement_state;
     // 根据当前状态决定调整哪个公差值
     // 通常在工程设置模式下进行公差设置
     
     // 使用measurement_state中的tolerance_setting_step字段跟踪当前设置的是上公差还是下公差
     switch(measurement_state.tolerance_setting_step) {
         case 0:
             // 设置上公差值
             measurement_state.tolerance_upper += increment;
             break;
         case 1:
             // 设置下公差值
             measurement_state.tolerance_lower += increment;
             break;
         default:
             measurement_state.tolerance_setting_step = 0;
             break;
     }
     
     ZTJ_UpdateLastActivity();
 }

 /**
  * @brief 切换公差设置步骤（上公差/下公差）
  * @param  None
  * @retval None
  */
 void ZTJ_ToggleToleranceSettingStep(void)
 {
     extern ZTJ_TypeDef measurement_state;
     // 切换公差设置步骤：0->1 或 1->0
     measurement_state.tolerance_setting_step = !measurement_state.tolerance_setting_step;
     ZTJ_UpdateLastActivity();
 }

 /**
 * @brief  获取TIR模式最大正跳动
 * @param  None
 * @retval 最大正跳动值
 */
int32_t ZTJ_GetTirMaxPositiveDeviation(void)
{
    return max_deviation_positive;
}

/**
 * @brief  获取TIR模式最大负跳动
 * @param  None
 * @retval 最大负跳动值
 */
int32_t ZTJ_GetTirMaxNegativeDeviation(void)
{
    return max_deviation_negative;
}

/**
 * @brief  获取TIR模式基准值
 * @param  None
 * @retval TIR基准值
 */
int32_t ZTJ_GetTirBaseValue(void)
{
    return tir_base_value;
}

/**
 * @brief  进入公差值设置模式，分别设置TOL_MAX(正公差)TOL_Min(负公差)
 * @param  None
 * @retval None
 */
void ZTJ_EnterToleranceSettingMode(void)
{
    extern ZTJ_TypeDef measurement_state;
    measurement_state.mode = MEASUREMENT_MODE_TOL_SETTING;
    ZTJ_UpdateLastActivity();
}

/**
 * @brief 切换公差检查状态（附加在ABS/REL模式中）
 * @param  None
 * @retval None
 */
void ZTJ_ToggleToleranceCheck(void)
{
    extern ZTJ_TypeDef measurement_state;
    // 仅在ABS或REL模式下允许切换公差检查状态
    if(measurement_state.mode == MEASUREMENT_MODE_ABS || measurement_state.mode == MEASUREMENT_MODE_REL) {
        // 切换公差检查状态
        measurement_state.is_in_tolerance_check = !measurement_state.is_in_tolerance_check;
        
        if(measurement_state.is_in_tolerance_check) {
            // 进入公差检查状态：保存当前显示值作为基准值
            extern SensorData_TypeDef averaged_sensor_value;
            measurement_state.tol_reference_value = averaged_sensor_value.value;
        } else {
            // 退出公差检查状态：清除基准值
            measurement_state.tol_reference_value = 0;
        }
    }
    ZTJ_UpdateLastActivity();
}