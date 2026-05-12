#ifndef __TRACE_FOLLOW_H
#define __TRACE_FOLLOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void TraceFollow_Init(void);
void TraceFollow_Start(void);
void TraceFollow_Stop(void);
void TraceFollow_Update(void);

void TraceFollow_SetSpeed(int16_t base_speed);
void TraceFollow_SetPid(float kp, float ki, float kd);
void TraceFollow_SetServoCenter(uint16_t center);
void TraceFollow_SetServoRange(uint16_t min, uint16_t max);
void TraceFollow_SetEncoderPid(float kp, float ki, float kd);
void TraceFollow_SetEncoderTarget(int16_t left_target, int16_t right_target);
void TraceFollow_GetAdc(uint16_t *left, uint16_t *middle, uint16_t *right);
float TraceFollow_GetError(void);
uint16_t TraceFollow_GetServoPulse(void);
void Bluetooth_SendTraceState(uint16_t left, uint16_t middle, uint16_t right, float error);

void TraceFollow_ManualStop(void);
void TraceFollow_ManualForward(int16_t speed);
void TraceFollow_ManualBackward(int16_t speed);
void TraceFollow_ManualLeft(int16_t speed);
void TraceFollow_ManualRight(int16_t speed);

void TraceFollow_HandleObstacle(uint32_t distance_cm);
uint8_t TraceFollow_IsObstacleActive(void);
uint8_t TraceFollow_GetObstaclePhase(void);

#ifdef __cplusplus
}
#endif

#endif
