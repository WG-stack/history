/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    data_processing.c
  * @brief   数据处理驱动文件
  ******************************************************************************
  * @attention
  *
  * 工作流程：
  * 1. 接收来自传感器采样的原始24位数据
  * 2. 提取符号位和数据位，生成带符号的原始值
  * 3. 使用滑动平均滤波算法对数据进行滤波处理（4次采样，去掉最大最小值后求平均）
  * 4. 结合测量状态机进行数值处理（校准+单位转换）
  * 5. 将处理后的数据封装为SensorData_TypeDef结构体返回
  *
  * 实现函数：
  * - DataProcessing_Init(): 初始化数据处理模块
  * - DataProcessing_Process(): 核心数据处理函数，接收原始数值并返回封装好的传感器数据结构体
  * - Data_Filter(): 滑动平均滤波函数
  * - Data_CheckValid(): 数据有效性校验函数
  * - DataProcessing_GetRawData(): 获取传感器原始数据函数
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
#include "data_processing.h"
#include "measurement_state_machine.h"
#include "main.h"
#include "stm32l1xx_hal.h"
#include <stdlib.h>
#include "global_variables.h"  // 包含传感器类型定义

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/**
 * @brief 传感器数据结构体定义
 */
typedef struct {
    int32_t raw_value;          // 原始采样值
    int32_t processed_value;    // 处理后的值
    bool is_valid;              // 数据有效性标志
} SensorRawData_TypeDef;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SENSOR_FILTER_COUNT           9  // 滑动平均滤波采样次数（改为4次）
#define SENSOR_DATA_MIN_VALUE        -999999// 传感器最小量程
#define SENSOR_DATA_MAX_VALUE        999999 // 传感器最大量程
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static SensorRawData_TypeDef sensor_raw_data = {0};
static int32_t sensor_filter_buffer[SENSOR_FILTER_COUNT] = {0};
static uint8_t filter_index = 0;
static ZTJ_TypeDef* measure_state_ptr = NULL;

// 按照用户数据处理流程定义的变量（已在文件开头定义，此处删除重复定义）

// 按照用户数据处理流程定义的变量
int32_t Rew_data = 0;          // 滤波后的数据 (Raw filtered data)
int32_t Abs_data = 0;          // 绝对值数据 (Absolute data)
int32_t Abs_bk_data = 0;       // 绝对基准点 (Absolute baseline)
int32_t Rer_data = 0;          // 相对值数据 (Relative data)
int32_t Rer_bk_data = 0;       // 相对基准点 (Relative baseline)
static uint8_t startup_cycle_count = 0;  // 开机循环计数，用于100个循环后自动清零
static uint8_t startup_auto_zero_completed = 0;  // 开机自动清零是否已完成（初始化标志）
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static int32_t Data_Filter(int32_t new_value);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief  滑动平均滤波函数（优化版：4次数据，去掉最大最小值后对剩余2个值求平均）
 * @param  new_value 新采样的原始值
 * @retval 滤波后的稳定值
 */
static int32_t Data_Filter(int32_t new_value)
{
    // 存入新值，更新缓冲区
    sensor_filter_buffer[filter_index++] = new_value;
    if(filter_index >= SENSOR_FILTER_COUNT)
    {
        filter_index = 0;
    }
    
    // 计算当前有多少个有效的数据
    uint8_t valid_count = 0;
    for(uint8_t i = 0; i < SENSOR_FILTER_COUNT; i++) {
        if(sensor_filter_buffer[i] != 0 || i < filter_index || (filter_index == 0 && i == SENSOR_FILTER_COUNT - 1)) {
            valid_count++;
        }
    }
    
    // 如果还没有4个有效值，则返回当前所有有效值的平均
    if(valid_count < SENSOR_FILTER_COUNT) {
        int32_t sum = 0;
        uint8_t actual_count = 0;
        for(uint8_t i = 0; i < SENSOR_FILTER_COUNT; i++) {
            if(sensor_filter_buffer[i] != 0 || i < filter_index || (filter_index == 0 && i == SENSOR_FILTER_COUNT - 1)) {
                sum += sensor_filter_buffer[i];
                actual_count++;
            }
        }
        return (actual_count > 0) ? sum / actual_count : new_value;
    }
    
    // 当有4个值时，找出最大值和最小值，然后对剩下的2个值求平均
    // 为了避免相等值被误判为最大最小值，我们明确找出最大值和最小值的索引
    int32_t min_val = sensor_filter_buffer[0];
    int32_t max_val = sensor_filter_buffer[0];
    uint8_t min_idx = 0;
    uint8_t max_idx = 0;
    
    for(uint8_t i = 1; i < SENSOR_FILTER_COUNT; i++) {
        if(sensor_filter_buffer[i] < min_val) {
            min_val = sensor_filter_buffer[i];
            min_idx = i;
        }
        if(sensor_filter_buffer[i] > max_val) {
            max_val = sensor_filter_buffer[i];
            max_idx = i;
        }
    }
    
    // 计算剩余值的平均值（排除最大值和最小值）
    int32_t sum = 0;
    uint8_t count = 0;
    for(uint8_t i = 0; i < SENSOR_FILTER_COUNT; i++) {
        if(i != min_idx && i != max_idx) {
            sum += sensor_filter_buffer[i];
            count++;
        }
    }
    
    // 如果恰好有两个相同的极值，count可能小于预期，此时使用安全的平均方式
    if(count > 0) {
        return sum / count;
    } else {
        // 万不得已的情况，返回新值
        return new_value;
    }
}

/**
 * @brief  重置滤波器，将所有缓冲区设置为指定值
 * @param  reset_value 重置值
 * @retval None
 */
void Data_Filter_Reset(int32_t reset_value)
{
    // 将滤波缓冲区的所有值设置为reset_value
    for(uint8_t i = 0; i < SENSOR_FILTER_COUNT; i++)
    {
        sensor_filter_buffer[i] = reset_value;
    }
    // 将索引重置为0
    filter_index = 0;
}

/* USER CODE END 0 */

/* Exported functions --------------------------------------------------------*/
/* USER CODE BEGIN EF */

/**
 * @brief 数据处理模块初始化函数
 * @param  None
 * @retval None
 */
void DataProcessing_Init(void)
{
    // 获取测量状态机指针
    extern ZTJ_TypeDef measurement_state;
    measure_state_ptr = &measurement_state;
    
    // 初始化滤波缓冲区（4次采样缓冲区）
    for(uint8_t i = 0; i < SENSOR_FILTER_COUNT; i++)
    {
        sensor_filter_buffer[i] = 0;
    }
    filter_index = 0;
    
    // 初始化原始数据结构体
    sensor_raw_data.raw_value = 0;
    sensor_raw_data.processed_value = 0;
    sensor_raw_data.is_valid = true;
    
    // 初始化用户数据处理流程变量
    Rew_data = 0;
    Abs_data = 0;
    Abs_bk_data = 0;
    Rer_data = 0;
    Rer_bk_data = 0;
    startup_cycle_count = 0;  // 重置开机循环计数
    startup_auto_zero_completed = 0;  // 重置开机自动清零标志
}

/**
 * @brief  核心数据处理函数
 * @param  raw_value 传感器采样的原始数值
 * @retval 封装好的传感器数据结构体
 */
SensorData_TypeDef DataProcessing_Process(int32_t raw_value)
{
    // 清空全局result_data变量
    result_data.is_valid = false;
    result_data.value = 0;
    result_data.sign = 0;
    result_data.unit = 0;
    result_data.mode = 0;
    for(uint8_t i = 0; i < 6; i++) {
        result_data.digits[i] = 0;
    }
    result_data.unit_metric = 0;
    result_data.last_digit = 0;
    
    // 1. 存入原始值
    sensor_raw_data.raw_value = raw_value;
    
    // 2. 按照用户数据处理流程：原始数据 -> 滤波后为"Rew_data"
    Rew_data = Data_Filter(raw_value);
    
    // 应用修正系数 xiuzhengXS
    Rew_data = (int32_t)((float)Rew_data * xiuzhengXS);

    

    // 3. 实现开机自动清绝对值零的逻辑（使用循环计数，在100个循环后执行）
    if (!startup_auto_zero_completed) {
        // 增加循环计数
        if (startup_cycle_count < 10) {
            startup_cycle_count++;
                    } else {
            // 达到100个循环后执行开机自动清零
            // Abs_bk_data=Rew_data，Abs_data=Rew_data-Abs_bk_data完成开机自动清绝对值零
            Abs_bk_data = Rew_data;
            Abs_data = Rew_data - Abs_bk_data;  // 这将为0，实现自动清零
            startup_auto_zero_completed = 1;    // 标记已完成开机自动清零
            
            // 更新测量状态机中的绝对零点偏移
            extern ZTJ_TypeDef measurement_state;
            measure_state_ptr = &measurement_state;
            measurement_state.absolute_zero_offset = Abs_bk_data;
            measurement_state.zero_offset = Abs_bk_data;
            
        }
    }
    
    // 检查是否需要手动清零校准（在按下ZERO键后设置的标志）
    extern uint8_t manual_zero_calibration_requested;  // 引用全局变量
    if (manual_zero_calibration_requested) {
        // 使用当前的滤波数据作为新的基准点
        Abs_bk_data = Rew_data;
        Abs_data = Rew_data - Abs_bk_data;  // 这将为0，实现手动清零
        manual_zero_calibration_requested = 0;   // 清除标志
        
        // 更新测量状态机中的基准点
        extern ZTJ_TypeDef measurement_state;
        measure_state_ptr = &measurement_state;
        measurement_state.absolute_zero_offset = Abs_bk_data;
        measurement_state.abs_bk_data = Abs_bk_data;  // 更新新的基准点字段
        measurement_state.zero_offset = Abs_bk_data;
        
    }

    // 4. 根据当前模式处理绝对值和相对值
    // 按照用户流程：绝对值使用数据"Abs_data"并设"Abs_bk_data"，相对值使用数据"Rer_data"并设"Rer_bk_data"
    
    // 先计算绝对值：Abs_data = Rew_data - Abs_bk_data
    // 注意：如果刚刚执行了手动清零校准(Abs_bk_data=Rew_data)，Abs_data已经是0
    // 否则，重新计算绝对值
    if (!manual_zero_calibration_requested) {  // 这个条件总是为真，因为我们已经在if块内将标志清零了
        Abs_data = Rew_data - Abs_bk_data;
    }
    // 实际上，上面的逻辑有问题，我们应该始终更新Abs_data以确保其反映当前的测量值
    Abs_data = Rew_data - Abs_bk_data;
    
    // 然后计算相对值：Rer_data = Rew_data - Rer_bk_data
    Rer_data = Rew_data - Rer_bk_data;
    
    switch (measure_state_ptr->mode) {
        case MEASUREMENT_MODE_ABS:
        case MEASUREMENT_MODE_NORMAL:
            // 绝对模式和正常模式：使用Abs_data作为显示值
            result_data.value = Abs_data;
            break;

        case MEASUREMENT_MODE_REL:
            // 相对模式：使用Rer_data作为显示值
            result_data.value = Rer_data;
            break;

        case MEASUREMENT_MODE_MAX:
            // MAX模式：基于Abs_data进行峰值检测
            // 最终显示值使用绝对值参与峰值计算
            result_data.value = Abs_data;
            break;

        case MEASUREMENT_MODE_MIN:
            // MIN模式：基于Abs_data进行峰值检测
            // 最终显示值使用绝对值参与峰值计算
            result_data.value = Abs_data;
            break;

        case MEASUREMENT_MODE_TIR:
            // TIR模式：基于Abs_data进行跳动值计算
            // 最终显示值使用绝对值参与跳动值计算
            result_data.value = Abs_data;
            break;

        case MEASUREMENT_MODE_SETUP:
            // 设置模式：基于Abs_data
            result_data.value = Abs_data;
            break;

        case MEASUREMENT_MODE_ENGINEER_SETUP:
            // 工程设置模式：基于Abs_data
            result_data.value = Abs_data;
            break;

        case MEASUREMENT_MODE_TOL_SETTING:
            // 公差设置模式：基于Abs_data
            result_data.value = Abs_data;
            break;

        default:
            // 未定义的模式，使用绝对值数据
            result_data.value = Abs_data;
            break;
    }

    // 5. 有效性校验 - 已移除以提高性能
    sensor_raw_data.is_valid = true; // 假设所有数据都有效

    // 6. 结合测量状态机处理数值（单位转换等，但基础值已按用户流程处理）
    extern ZTJ_TypeDef measurement_state;
    int32_t temp_processed_value = ZTJ_UpdateMeasureValue(measure_state_ptr, result_data.value);
    result_data.value = temp_processed_value;
    sensor_raw_data.processed_value = temp_processed_value;
    
    // 更新全局平均传感器值，用于零点校准（现在这个值已经考虑了正负号）
    averaged_sensor_value.value = temp_processed_value;
    averaged_sensor_value.sign = result_data.sign;  // 同时保存符号信息
    averaged_sensor_value.unit = result_data.unit;  // 同时保存单位信息
    averaged_sensor_value.mode = result_data.mode;  // 同时保存模式信息
    averaged_sensor_value.is_valid = result_data.is_valid;  // 同时保存有效性信息
    // 复制数字数组
    for(uint8_t i = 0; i < 6; i++) {
        averaged_sensor_value.digits[i] = result_data.digits[i];
    }
    averaged_sensor_value.unit_metric = result_data.unit_metric;
    averaged_sensor_value.last_digit = result_data.last_digit;
    
    // 更新LCD显示状态机的模式信息
    // 注意：这里不应该覆盖按键设置的单位状态，只在必要时更新模式
    extern void LCD_DisplayState_SetMode(uint8_t mode);
    LCD_DisplayState_SetMode(result_data.mode);
    
    // 7. 根据传感器类型和显示分辨率进行数据转换
    // 传感器分辨率为两种，一种为0.5u，一种是1u
    // 如果使用0.5u的传感器：
    //   - 0.5u显示分辨率时，做*5计算
    //   - 1u显示分辨率时，不做计算
    //   - 0.01mm显示分辨率时，做/20
    // 如果使用1u的传感器：
    //   - 0.5u显示分辨率时，做*2计算
    //   - 1u显示分辨率时，不做计算
    //   - 0.01mm显示分辨率时，做/10
    int32_t converted_value = result_data.value;
    
    switch(measure_state_ptr->current_resolution) {
        case RESOLUTION_0_0005MM:  // 0.5u显示分辨率
            if(sensor_type == SENSOR_TYPE_0_5UM) {
                // 0.5u传感器 + 0.5u显示分辨率：做*5计算
                converted_value = result_data.value * 5;
            } else if(sensor_type == SENSOR_TYPE_1UM) {
                // 1u传感器 + 0.5u显示分辨率：不做计算
                converted_value = result_data.value;
            }
            break;
            
        case RESOLUTION_0_001MM:  // 1u显示分辨率
            if(sensor_type == SENSOR_TYPE_0_5UM) {
                // 0.5u传感器 + 1u显示分辨率：做/2
                converted_value = result_data.value / 2;
            } else if(sensor_type == SENSOR_TYPE_1UM) {
                // 1u传感器 + 1u显示分辨率：不做计算
                converted_value = result_data.value / 2;
            }
            break;
            
        case RESOLUTION_0_01MM:  // 0.01mm显示分辨率
            if(sensor_type == SENSOR_TYPE_0_5UM) {
                // 0.5u传感器 + 0.01mm显示分辨率：做/20
                converted_value = result_data.value / 20;
            } else if(sensor_type == SENSOR_TYPE_1UM) {
                // 1u传感器 + 0.01mm显示分辨率：做/10
                converted_value = result_data.value / 10;
            }
            break;
            
        default:
            converted_value = result_data.value;
            break;
    }
    
    // 7. 封装输出结构体
    result_data.is_valid = true;
    
    // 根据处理后的值确定符号（而非硬件符号位），确保符号与数值一致
    if(converted_value < 0) {
        result_data.sign = 1;  // 负数
    } else {
        result_data.sign = 0;  // 正数或零
    }
    
    // 更新sensor_raw_data.processed_value以反映带符号的值
    sensor_raw_data.processed_value = converted_value;
    
    result_data.value = converted_value;  // 使用转换后的值
    result_data.unit = measure_state_ptr->unit;
    result_data.mode = measure_state_ptr->mode;
    // 数字拆分：把value拆成6位数字存入digits
    int32_t temp_val = result_data.value;
    // 处理负数情况
    if(temp_val < 0) {
        temp_val = -temp_val; // 使用绝对值进行数字分解
    }
    for(uint8_t i = 0; i < 6; i++) {
        result_data.digits[5 - i] = temp_val % 10;
        temp_val /= 10;
    }
    // 单位映射：unit转unit_metric
    result_data.unit_metric = (result_data.unit == 0) ? 1 : 0; // 根据实际逻辑调整
    // 最后一位小数赋值
    result_data.last_digit = result_data.digits[0];
    
    // 更新模拟指针值
    DataProcessing_UpdateAnalogPointer(result_data.value, (Resolution_TypeDef)measure_state_ptr->current_resolution);
    
    // 确保last_digit与模拟指针值保持一致
    // 当模拟指针值为正时，last_digit表示点亮的符号数量
    // 当模拟指针值为负时，sign位表示负号，last_digit表示点亮的符号数量
    if(analog_pointer_value >= 0) {
        result_data.last_digit = (uint8_t)analog_pointer_value;
        result_data.sign = 0;  // 正数
    } else {
        result_data.last_digit = (uint8_t)(-analog_pointer_value);  // 取绝对值
        result_data.sign = 1;  // 负数
    }
    
    return result_data;
}

/**
 * @brief  获取传感器原始数据
 * @param  None
 * @retval 原始数据结构体
 */
SensorRawData_TypeDef DataProcessing_GetRawData(void)
{
    return sensor_raw_data;
}
/* USER CODE END EF */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/**
 * @brief 设置绝对基准点
 * @param baseline_value 基准值
 * @retval None
 */
void DataProcessing_SetAbsBaseline(int32_t baseline_value)
{
    Abs_bk_data = baseline_value;
}

/**
 * @brief 使用最新的滤波数据设置绝对基准点
 * @param None
 * @retval None
 */
void DataProcessing_SetAbsBaselineWithFilteredData(void)
{
    // 使用最新的滤波数据作为基准点
    Abs_bk_data = Rew_data;
}

/**
 * @brief 从当前绝对值设置相对基准点
 * @param None
 * @retval None
 */
void DataProcessing_SetRelBaselineFromAbs(void)
{
    // 使用当前的滤波数据作为相对基准点，以确保切换到相对模式时显示为0
    // Rer_data = Rew_data - Rer_bk_data，若要Rer_data为0，则Rer_bk_data应等于Rew_data
    Rer_bk_data = Rew_data;
}

/**
 * @brief 获取当前绝对值数据
 * @param None
 * @retval 当前绝对值数据
 */
int32_t DataProcessing_GetAbsData(void)
{
    return Abs_data;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 1 */

/**
 * @brief 计算模拟指针显示值
 * @param input_value 输入的测量值
 * @param resolution 当前分辨率
 * @retval 返回模拟指针值 (-15 到 +15)
 */
int8_t DataProcessing_CalculateAnalogPointerValue(int32_t input_value, Resolution_TypeDef resolution)
{
    // 根据不同分辨率计算指针值
    float scaled_value = 0.0f;
    
    // 使用更合理的缩放因子，确保指针值在有效范围内
    switch(resolution) {
        case RESOLUTION_0_0005MM:  // 0.0005mm分辨率
            // 每0.005mm对应一个指针单位，这样在15个单位时对应0.075mm
            scaled_value = (float)input_value * 0.001f; // 调整缩放因子
            break;
        case RESOLUTION_0_001MM:   // 0.001mm分辨率
            // 每0.01mm对应一个指针单位，这样在15个单位时对应0.15mm
            scaled_value = (float)input_value * 0.1f; // 调整缩放因子
            break;
        case RESOLUTION_0_01MM:    // 0.01mm分辨率
            // 每0.15mm对应一个指针单位，这样在15个单位时对应2.25mm
            scaled_value = (float)input_value * 0.067f; // 调整缩放因子
            break;
        default:
            // 默认使用0.001mm的缩放因子
            scaled_value = (float)input_value * 0.001f;
            break;
    }
    
    // 严格限制值在-15到+15范围内，确保不会超出指针图标范围
    if(scaled_value > 15.0f) {
        scaled_value = 15.0f;
    } else if(scaled_value < -15.0f) {
        scaled_value = -15.0f;
    }
    
    // 确保返回值在int8_t范围内
    int8_t result = (int8_t)scaled_value;
    if(result > 15) result = 15;
    if(result < -15) result = -15;
    
    return result;
}

/**
 * @brief 更新模拟指针全局变量
 * @param input_value 输入的测量值
 * @param resolution 当前分辨率
 * @retval None
 */
void DataProcessing_UpdateAnalogPointer(int32_t input_value, Resolution_TypeDef resolution)
{
    // 计算模拟指针值
    analog_pointer_value = DataProcessing_CalculateAnalogPointerValue(input_value, resolution);
}

/* USER CODE END 1 */
