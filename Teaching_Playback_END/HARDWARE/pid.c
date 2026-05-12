/**
 * @file    pid.c
 * @brief   PID控制器实现
 */

#include "pid.h"

void PID_Init(PID_t *pid, float kp, float ki, float kd, float out_max)
{
    pid->kp             = kp;
    pid->ki             = ki;
    pid->kd             = kd;
    pid->out_max        = out_max;
    pid->integral       = 0.0f;
    pid->prev_error     = 0.0f;

    /*
     * 积分项限幅: 防止Ki过小时积分无限增大
     * 限幅值设为 out_max, 确保积分单独贡献不超过输出限幅
     */
    pid->integral_limit = (ki > 1e-6f) ? (out_max / ki) : out_max;
}

float PID_Calculate(PID_t *pid, float setpoint, float measured)
{
    float error = setpoint - measured;

    /* 积分累积 */
    pid->integral += error;

    /* 积分限幅 (抗积分饱和) */
    if      (pid->integral >  pid->integral_limit) pid->integral =  pid->integral_limit;
    else if (pid->integral < -pid->integral_limit) pid->integral = -pid->integral_limit;

    /* 微分 (基于误差变化) */
    float derivative = error - pid->prev_error;
    pid->prev_error  = error;

    /* 计算输出 */
    float output = pid->kp * error
                 + pid->ki * pid->integral
                 + pid->kd * derivative;

    /* 输出限幅 */
    if      (output >  pid->out_max) output =  pid->out_max;
    else if (output < -pid->out_max) output = -pid->out_max;

    return output;
}

void PID_Reset(PID_t *pid)
{
    pid->integral   = 0.0f;
    pid->prev_error = 0.0f;
}

void PID_SetParams(PID_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integral_limit = (ki > 1e-6f) ? (pid->out_max / ki) : pid->out_max;
}
