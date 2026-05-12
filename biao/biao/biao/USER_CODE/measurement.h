/**
  ******************************************************************************
  * @file           : measurement.h
  * @brief          : 测量逻辑模块头文件
  * @author         : 量博科技
  * @date           : 2026-01-07
  ******************************************************************************
  * @attention
  * 
  * 测量模式：
  * - ABS: 绝对测量
  * - REL: 相对测量
  * - MAX: 最大值
  * - MIN: 最小值
  * - TIR: 跳动值 (MAX-MIN)
  * - TOL: 公差判断
  * 
  ******************************************************************************
  */

#ifndef __MEASUREMENT_H
#define __MEASUREMENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "sensor_gpio.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    MODE_ABS = 0,   // 绝对测量
    MODE_REL,       // 相对测量
    MODE_MAX,       // 最大值
    MODE_MIN,       // 最小值
    MODE_TIR        // 跳动值
} MeasureMode_t;

typedef enum {
    RESOLUTION_0_0005MM = 0,  // 0.0005mm (万分表)
    RESOLUTION_0_001MM,       // 0.001mm  (千分表)
    RESOLUTION_0_01MM         // 0.01mm   (百分表)
} Resolution_t;

typedef enum {
    UNIT_MM = 0,    // 公制
    UNIT_INCH       // 英制
} Unit_t;

typedef enum {
    TOL_NONE = 0,   // 无公差判断
    TOL_OK,         // 在公差范围内
    TOL_UNDER,      // 低于下限 (>|)
    TOL_OVER        // 超过上限 (|<)
} TolStatus_t;

typedef struct {
    float current_value;        // 当前显示值
    float abs_zero_value;       // 绝对零点值
    float rel_zero_value;       // 相对零点值
    float max_value;            // 最大值
    float min_value;            // 最小值
    float tir_value;            // 跳动值
    
    MeasureMode_t mode;         // 测量模式
    Resolution_t resolution;    // 分辨率
    Unit_t unit;                // 单位
    
    // 公差设置
    uint8_t tol_enabled;        // 公差使能
    float tol_standard;         // 标准值
    float tol_upper;            // 上公差
    float tol_lower;            // 下公差
    TolStatus_t tol_status;     // 公差状态
    
    // 指针显示
    int8_t pointer_value;       // 指针值 (-15~+15, 0, ++, --)
    
    uint8_t data_updated;       // 数据更新标志
} MeasureData_t;

/* Exported functions prototypes ---------------------------------------------*/
void Measure_Init(void);
void Measure_Process(SensorData_t *sensor_data);
void Measure_SetMode(MeasureMode_t mode);
void Measure_SetResolution(Resolution_t res);
void Measure_SetUnit(Unit_t unit);
void Measure_Zero(void);
void Measure_SetTolerance(float standard, float upper, float lower);
void Measure_ClearTolerance(void);
MeasureData_t* Measure_GetData(void);

#ifdef __cplusplus
}
#endif

#endif /* __MEASUREMENT_H */
