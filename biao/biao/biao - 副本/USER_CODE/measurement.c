/**
  ******************************************************************************
  * @file           : measurement.c
  * @brief          : 测量逻辑模块实现
  ******************************************************************************
  */

#include "measurement.h"
#include <string.h>
#include <math.h>

/* Private variables ---------------------------------------------------------*/
static MeasureData_t measure_data;

/* Private function prototypes -----------------------------------------------*/
static float ConvertSensorValue(SensorData_t *sensor_data);
static void UpdatePointer(void);
static void CheckTolerance(void);

/**
  * @brief  测量模块初始化
  */
void Measure_Init(void)
{
    memset(&measure_data, 0, sizeof(MeasureData_t));
    
    measure_data.mode = MODE_ABS;
    measure_data.resolution = RESOLUTION_0_001MM;
    measure_data.unit = UNIT_MM;
    measure_data.tol_enabled = 0;
}

/**
  * @brief  处理传感器数据
  */
void Measure_Process(SensorData_t *sensor_data)
{
    if (!sensor_data->data_valid) {
        return;
    }
    
    // 转换传感器值
    float sensor_value = ConvertSensorValue(sensor_data);
    
    // 根据模式处理数据
    switch (measure_data.mode) {
        case MODE_ABS:
            measure_data.current_value = sensor_value - measure_data.abs_zero_value;
            break;
            
        case MODE_REL:
            measure_data.current_value = sensor_value - measure_data.rel_zero_value;
            break;
            
        case MODE_MAX:
            if (sensor_value > measure_data.max_value) {
                measure_data.max_value = sensor_value;
            }
            measure_data.current_value = measure_data.max_value;
            break;
            
        case MODE_MIN:
            if (sensor_value < measure_data.min_value) {
                measure_data.min_value = sensor_value;
            }
            measure_data.current_value = measure_data.min_value;
            break;
            
        case MODE_TIR:
            if (sensor_value > measure_data.max_value) {
                measure_data.max_value = sensor_value;
            }
            if (sensor_value < measure_data.min_value) {
                measure_data.min_value = sensor_value;
            }
            measure_data.tir_value = measure_data.max_value - measure_data.min_value;
            measure_data.current_value = measure_data.tir_value;
            break;
    }
    
    // 更新指针显示
    UpdatePointer();
    
    // 检查公差
    if (measure_data.tol_enabled) {
        CheckTolerance();
    }
    
    measure_data.data_updated = 1;
}

/**
  * @brief  转换传感器值
  * @note   传感器输出：每个数0.5微米
  */
static float ConvertSensorValue(SensorData_t *sensor_data)
{
    float value = sensor_data->value * 0.0005f;  // 转换为mm
    
    // 单位转换
    if (measure_data.unit == UNIT_INCH) {
        value = value / 25.4f;  // mm转inch
    }
    
    return value;
}

/**
  * @brief  更新指针显示
  * @note   根据分辨率和当前值计算指针位置
  */
static void UpdatePointer(void)
{
    float fractional_part;
    int pointer;
    
    // 获取小数部分
    fractional_part = fabsf(measure_data.current_value) - floorf(fabsf(measure_data.current_value));
    
    // 根据分辨率计算指针
    switch (measure_data.resolution) {
        case RESOLUTION_0_0005MM:  // 万分表
            pointer = (int)(fractional_part * 10000) % 10;
            break;
            
        case RESOLUTION_0_001MM:   // 千分表
            pointer = (int)(fractional_part * 1000) % 10;
            break;
            
        case RESOLUTION_0_01MM:    // 百分表
            pointer = (int)(fractional_part * 100) % 10;
            break;
            
        default:
            pointer = 0;
            break;
    }
    
    // 处理超范围
    if (pointer > 15) {
        measure_data.pointer_value = 16;  // ps++
    } else if (pointer < -15) {
        measure_data.pointer_value = -16; // ps--
    } else {
        measure_data.pointer_value = pointer;
    }
}

/**
  * @brief  检查公差
  */
static void CheckTolerance(void)
{
    float deviation = measure_data.current_value - measure_data.tol_standard;
    
    if (deviation < measure_data.tol_lower) {
        measure_data.tol_status = TOL_UNDER;
    } else if (deviation > measure_data.tol_upper) {
        measure_data.tol_status = TOL_OVER;
    } else {
        measure_data.tol_status = TOL_OK;
    }
}

/**
  * @brief  设置测量模式
  */
void Measure_SetMode(MeasureMode_t mode)
{
    measure_data.mode = mode;
    
    // 重置MAX/MIN/TIR基准值
    if (mode == MODE_MAX || mode == MODE_MIN || mode == MODE_TIR) {
        measure_data.max_value = measure_data.current_value;
        measure_data.min_value = measure_data.current_value;
        measure_data.tir_value = 0;
    }
}

/**
  * @brief  设置分辨率
  */
void Measure_SetResolution(Resolution_t res)
{
    measure_data.resolution = res;
}

/**
  * @brief  设置单位
  */
void Measure_SetUnit(Unit_t unit)
{
    measure_data.unit = unit;
}

/**
  * @brief  清零
  */
void Measure_Zero(void)
{
    if (measure_data.mode == MODE_ABS) {
        measure_data.abs_zero_value += measure_data.current_value;
    } else {
        measure_data.rel_zero_value = measure_data.current_value;
    }
}

/**
  * @brief  设置公差
  */
void Measure_SetTolerance(float standard, float upper, float lower)
{
    measure_data.tol_enabled = 1;
    measure_data.tol_standard = standard;
    measure_data.tol_upper = upper;
    measure_data.tol_lower = lower;
}

/**
  * @brief  清除公差
  */
void Measure_ClearTolerance(void)
{
    measure_data.tol_enabled = 0;
}

/**
  * @brief  获取测量数据
  */
MeasureData_t* Measure_GetData(void)
{
    return &measure_data;
}
