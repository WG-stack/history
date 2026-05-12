/**
  ******************************************************************************
  * @file           : measurement.c
  * @brief          : 测量逻辑模块实现
  ******************************************************************************
  */

#include "measurement.h"
#include "settings.h"
#include <string.h>
#include <math.h>

/* Private variables ---------------------------------------------------------*/
static MeasureData_t measure_data;

//为了解决抖动数值跳动
static float last_value = 0.0f;
static const float MIN_DELTA = 0.0002f;  // 根据分辨率选择合适的值

// 平均滤波缓冲
#define MAX_AVG_TIMES 16
static float avg_buffer[MAX_AVG_TIMES];
static uint8_t avg_index = 0;
static uint8_t avg_count = 0;
static AverageTimes_t last_avg_setting = (AverageTimes_t)-1; // 初始化为一个无效值

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
    measure_data.resolution = RESOLUTION_0_01MM;
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
    
    // 获取平均设置
    SystemSettings_t *settings = Settings_Get();
    uint8_t target_avg_count = 1;
    switch (settings->avg_times) {
        case AVG_TIMES_0: target_avg_count = 1; break;
        case AVG_TIMES_4: target_avg_count = 4; break;
        case AVG_TIMES_8: target_avg_count = 8; break;
        case AVG_TIMES_16: target_avg_count = 16; break;
    }
    
    // 如果平均次数设置改变，或者刚开机，重置缓冲区
    if (settings->avg_times != last_avg_setting || avg_count == 0) {
        for (int i = 0; i < MAX_AVG_TIMES; i++) {
            avg_buffer[i] = sensor_value;
        }
        avg_count = target_avg_count;
        avg_index = 0;
        last_avg_setting = settings->avg_times;
    }
    
    // 存入缓冲区
    avg_buffer[avg_index] = sensor_value;
    avg_index = (avg_index + 1) % target_avg_count;
    
    // 计算平均值
    float sum = 0.0f;
    for (int i = 0; i < target_avg_count; i++) {
        sum += avg_buffer[i];
    }
    sensor_value = sum / target_avg_count;
    
    float new_value;
    if (measure_data.mode == MODE_ABS) {
        new_value = sensor_value - measure_data.abs_zero_value;
    } else if (measure_data.mode == MODE_REL) {
        new_value = sensor_value - measure_data.rel_zero_value;
    } else {
        // 其它模式沿用你现有逻辑
    }
    // 死区判断：小变化直接忽略
    // if (fabsf(new_value - last_value) < MIN_DELTA) {
    //     return;            // 不更新 current_value / 显示
    // }
    // measure_data.current_value = new_value;
    // last_value = new_value;
    // 死区判断：直接使用滤波后的 sensor_value 进行判断，适用于所有模式
    if (fabsf(sensor_value - last_value) < MIN_DELTA) {
        return;            // 变化太小，不更新显示
    }
    last_value = sensor_value;
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
/*     SystemSettings_t *settings = Settings_Get();
    float value = sensor_data->value * 0.0005f;  // 转换为mm

    if (settings->move_direction == DIR_DOWN_POSITIVE) {
        value = -value;
    }
    
    // 单位转换
    if (measure_data.unit == UNIT_INCH) {
        value = value / 25.4f;  // mm转inch
    }
    
    return value; */
    SystemSettings_t *settings = Settings_Get();
    
    // 1. 直接从 raw_data 提取 20 位原始数值，避免 sensor_gpio.c 中的重复缩放
    uint32_t raw_val = sensor_data->raw_data & 0x0FFFFF;
    
    // 根据传感器类型决定倍率
    float value;
    if (settings->sensor_type == SENSOR_TYPE_0_0005MM) {
        value = (float)raw_val * 0.0005f;  // 转换为mm (每个数0.5微米)
    } else {
        value = (float)raw_val * 0.001f;   // 转换为mm (每个数1微米)
    }
    
    // 2. 恢复符号
    if (sensor_data->is_negative) {
        value = -value;
    }
    // 3. 方向设置
    if (settings->move_direction == DIR_DOWN_POSITIVE) {
        value = -value;
    }
    
    // 4. 单位转换
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
    MeasureMode_t old_mode = measure_data.mode;
    // 先根据旧模式和新模式做特殊处理
    if (old_mode == MODE_ABS && mode == MODE_REL) {
        // 推算当前的实际传感器值
        float sensor_value = measure_data.current_value + measure_data.abs_zero_value;
        // 把这个位置作为相对零位
        measure_data.rel_zero_value = sensor_value;
        // 为了立即显示为0
        measure_data.current_value = 0.0f;
        //measure_data.data_updated = 1;  // 让显示逻辑立刻刷新
    }
    
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
        measure_data.rel_zero_value += measure_data.current_value;
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
