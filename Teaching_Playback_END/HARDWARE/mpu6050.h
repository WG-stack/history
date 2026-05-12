/**
 * @file    mpu6050.h
 * @brief   MPU6050 六轴IMU驱动 (I2C1接口)
 *
 * 硬件连接:
 *   SDA -> PB7 (I2C1_SDA)
 *   SCL -> PB6 (I2C1_SCL)
 *   AD0 -> GND  (I2C地址 0x68)
 *   INT -> 未接 (轮询模式)
 *
 * 功能:
 *   - 读取加速度计 / 陀螺仪原始数据
 *   - 积分陀螺仪Z轴角速度获取航向角 (偏航角 Yaw)
 *   - 用于再现阶段的航向角辅助校正
 */

#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "main.h"

/* MPU6050 I2C地址 (AD0=GND -> 0x68, 左移1位后 = 0xD0) */
#define MPU6050_I2C_ADDR    0xD0U

typedef struct {
    int16_t ax;       /* 加速度计 X 原始值 */
    int16_t ay;       /* 加速度计 Y 原始值 */
    int16_t az;       /* 加速度计 Z 原始值 */
    int16_t gx;       /* 陀螺仪 X 原始值 */
    int16_t gy;       /* 陀螺仪 Y 原始值 */
    int16_t gz;       /* 陀螺仪 Z 原始值 */
    float   temp;     /* 温度 (°C) */
    float   yaw;      /* 航向角 (°), 陀螺仪Z轴积分 */
    float   gz_bias;  /* 陀螺仪Z轴零偏 (校准值) */
} MPU6050_Data_t;

/**
 * @brief  初始化MPU6050
 * @return 1=成功, 0=通信失败 (检查接线)
 */
uint8_t MPU6050_Init(void);

/**
 * @brief  读取所有传感器原始数据 (14字节一次性读取)
 * @param  data  输出结构体指针
 */
void MPU6050_ReadRaw(MPU6050_Data_t *data);

/**
 * @brief  更新航向角 (积分陀螺仪Z轴)
 * @param  data   数据结构指针 (需已读取原始数据)
 * @param  dt_s   采样周期 (秒), 如 0.01f 代表10ms
 */
void MPU6050_UpdateYaw(MPU6050_Data_t *data, float dt_s);

/**
 * @brief  静止校准陀螺仪Z轴零偏 (调用时小车必须静止)
 * @param  data     数据结构指针
 * @param  samples  采样次数 (建议100~200)
 */
void MPU6050_CalibrateGyro(MPU6050_Data_t *data, uint16_t samples);

/**
 * @brief  复位航向角为0
 */
void MPU6050_ResetYaw(MPU6050_Data_t *data);

#endif /* __MPU6050_H__ */
