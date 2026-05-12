/**
 * @file    pid.h
 * @brief   增量式速度PID控制器
 *
 * 控制目标: 轮速跟踪 (单位: 编码器脉冲/采样周期)
 *
 * 公式:
 *   e(k)     = setpoint - measured
 *   output   = Kp*e(k) + Ki*sum(e) + Kd*(e(k)-e(k-1))
 *
 * 防饱和措施:
 *   - 积分项限幅 (integral_limit)
 *   - 输出限幅 (out_max)
 *
 * 推荐初始参数 (根据实际电机需自整定):
 *   Kp=5.0, Ki=0.3, Kd=0.1, out_max=999
 */

#ifndef __PID_H__
#define __PID_H__

#include "main.h"

typedef struct {
    float kp;             /* 比例系数 */
    float ki;             /* 积分系数 */
    float kd;             /* 微分系数 */
    float integral;       /* 积分累积值 */
    float prev_error;     /* 上一次误差 (用于微分) */
    float out_max;        /* 输出上限 (含符号) */
    float integral_limit; /* 积分项限幅 */
} PID_t;

/**
 * @brief  初始化PID参数
 * @param  pid      PID结构体指针
 * @param  kp       比例系数
 * @param  ki       积分系数
 * @param  kd       微分系数
 * @param  out_max  输出绝对值上限 (如999.0f)
 */
void PID_Init(PID_t *pid, float kp, float ki, float kd, float out_max);

/**
 * @brief  计算PID输出
 * @param  pid       PID结构体指针
 * @param  setpoint  目标值 (目标速度)
 * @param  measured  实测值 (实际速度)
 * @return 输出量 (已限幅在 [-out_max, +out_max])
 */
float PID_Calculate(PID_t *pid, float setpoint, float measured);

/**
 * @brief  复位PID内部状态 (积分项和上一次误差清零)
 */
void PID_Reset(PID_t *pid);

/**
 * @brief  动态修改PID参数 (运行中调整)
 */
void PID_SetParams(PID_t *pid, float kp, float ki, float kd);

#endif /* __PID_H__ */
