#include "trace_follow.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "ultrasonic.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define TRACE_ADC_COUNT 3
#define TRACE_PWM_PERIOD_MAX 999
#define TRACE_ERROR_DEADBAND 0.03f
#define TRACE_SERVO_SWITCH_THRESHOLD 0.4f//这个值如果直线时舵机幅度大，可以适当提高，舵机只用在偏移较大时做修正
#define TRACE_SERVO_GAIN 12.0f//这个值越小舵机转向越柔和

#define TRACE_OBSTACLE_STOP_CM 50U
#define TRACE_OBSTACLE_SCAN_LEFT_MS 700U
#define TRACE_OBSTACLE_SCAN_RIGHT_MS 700U
#define TRACE_OBSTACLE_FORWARD_MS 600U
#define TRACE_OBSTACLE_SETTLE_MS 200U

typedef struct
{
  float kp;
  float ki;
  float kd;
  float integral;
  float last_error;
} PidController;

typedef enum
{
  OBSTACLE_IDLE = 0,
  OBSTACLE_STOPPING,
  OBSTACLE_TURN_LEFT,
  OBSTACLE_SAMPLE_LEFT,
  OBSTACLE_TURN_RIGHT,
  OBSTACLE_SAMPLE_RIGHT,
  OBSTACLE_DECIDE,
  OBSTACLE_FORWARD,
  OBSTACLE_RESUME
} ObstacleState;

static uint16_t s_trace_adc_dma[TRACE_ADC_COUNT] = {0};
static float s_current_error = 0.0f;
static int16_t s_base_speed = 300;
static uint16_t s_servo_center = 1500;
static uint16_t s_servo_min = 1000;
static uint16_t s_servo_max = 2000;
static uint16_t s_servo_current = 1500;

static PidController s_trace_pid = {10.0f, 0.0f, 2.0f, 0.0f, 0.0f};//舵机PID
static PidController s_left_speed_pid = {0.6f, 0.0f, 0.0f, 0.0f, 0.0f};//左轮PID{1.2f, 0.02f, 0.01f, 0.0f, 0.0f};
static PidController s_right_speed_pid = {0.6f, 0.0f, 0.0f, 0.0f, 0.0f};//右轮PID

static int16_t s_left_target_speed = 0;
static int16_t s_right_target_speed = 0;
static int16_t s_left_pwm = 0;
static int16_t s_right_pwm = 0;
static int32_t s_left_last_pos = 0;
static int32_t s_right_last_pos = 0;

static ObstacleState s_obstacle_state = OBSTACLE_IDLE;
static uint32_t s_obstacle_enter_tick = 0;
static uint32_t s_obstacle_phase_tick = 0;
static uint16_t s_obstacle_left_distance = 0;
static uint16_t s_obstacle_right_distance = 0;
static int16_t s_saved_base_speed = 300;

static void TraceFollow_SetMotorDirection(int8_t left_dir, int8_t right_dir)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, left_dir > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, left_dir > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, right_dir > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, right_dir > 0 ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void TraceFollow_SetPwm(int16_t left_pwm, int16_t right_pwm)
{
  if (left_pwm < 0) left_pwm = 0;
  if (right_pwm < 0) right_pwm = 0;
  if (left_pwm > TRACE_PWM_PERIOD_MAX) left_pwm = TRACE_PWM_PERIOD_MAX;
  if (right_pwm > TRACE_PWM_PERIOD_MAX) right_pwm = TRACE_PWM_PERIOD_MAX;
  s_left_pwm = left_pwm;
  s_right_pwm = right_pwm;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint16_t)left_pwm);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, (uint16_t)right_pwm);
}

static void TraceFollow_SetServoPulse(uint16_t pulse)
{
  if (pulse < s_servo_min) pulse = s_servo_min;
  if (pulse > s_servo_max) pulse = s_servo_max;
  s_servo_current = pulse;
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
}

static void TraceFollow_ReadAdc(uint16_t *left, uint16_t *middle, uint16_t *right)
{
  if (left != NULL) *left = s_trace_adc_dma[0];
  if (middle != NULL) *middle = s_trace_adc_dma[1];
  if (right != NULL) *right = s_trace_adc_dma[2];
}

static float TraceFollow_ComputeError(uint16_t left, uint16_t middle, uint16_t right)
{
  float l = (float)left;
  float m = (float)middle;
  float r = (float)right;
  float sum = l + m + r;
  float error;

  if (sum < 1.0f)
  {
    return 0.0f;
  }

  error = (r - l) / sum;
  if (fabsf(error) < TRACE_ERROR_DEADBAND)
  {
    error = 0.0f;
  }
  return error;
}

static float Pid_Compute(PidController *pid, float error)
{
  float derivative = error - pid->last_error;
  pid->integral += error;
  if (pid->integral > 1000.0f) pid->integral = 1000.0f;
  if (pid->integral < -1000.0f) pid->integral = -1000.0f;
  pid->last_error = error;
  return pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;
}

static void Pid_Reset(PidController *pid)
{
  pid->integral = 0.0f;
  pid->last_error = 0.0f;
}

static int16_t TraceFollow_GetEncoderSpeed(TIM_HandleTypeDef *htim, int32_t *last_pos)
{
  int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(htim);
  int32_t delta = now - *last_pos;
  *last_pos = now;
  return (int16_t)delta;
}

static void TraceFollow_UpdateSpeedLoop(void)//电机速度PID
{
  int16_t left_speed = TraceFollow_GetEncoderSpeed(&htim2, &s_left_last_pos);
  int16_t right_speed = TraceFollow_GetEncoderSpeed(&htim4, &s_right_last_pos);
  float left_pwm_delta = 0.0f;
  float right_pwm_delta = 0.0f;

  left_pwm_delta = Pid_Compute(&s_left_speed_pid, (float)(s_left_target_speed - left_speed));
  right_pwm_delta = Pid_Compute(&s_right_speed_pid, (float)(s_right_target_speed - right_speed));
  if (left_pwm_delta > 15.0f) left_pwm_delta = 15.0f;//输出限幅
  if (left_pwm_delta < -15.0f) left_pwm_delta = -15.0f;
  if (right_pwm_delta > 15.0f) right_pwm_delta = 15.0f;
  if (right_pwm_delta < -15.0f) right_pwm_delta = -15.0f;
  TraceFollow_SetPwm((int16_t)(s_left_pwm + left_pwm_delta), (int16_t)(s_right_pwm + right_pwm_delta));
}

static void TraceFollow_UpdateSteering(float error)//循迹PID，用于循迹偏差控制根据电磁三路 AD 算出来的 error计算舵机转向量，让车在大偏差时转向修
{
  float pid_out = Pid_Compute(&s_trace_pid, error);

  if (fabsf(error) > TRACE_SERVO_SWITCH_THRESHOLD)
  {
    int32_t target = (int32_t)s_servo_center + (int32_t)(pid_out * TRACE_SERVO_GAIN);
    if (target < (int32_t)s_servo_min) target = s_servo_min;
    if (target > (int32_t)s_servo_max) target = s_servo_max;
    TraceFollow_SetServoPulse((uint16_t)target);
  }
  else
  {
    TraceFollow_SetServoPulse(s_servo_center);
  }
}

static void TraceFollow_ApplyStop(void)
{
  TraceFollow_SetPwm(0, 0);
  TraceFollow_SetServoPulse(s_servo_center);
  TraceFollow_SetMotorDirection(1, 1);
}

static void TraceFollow_ApplyResume(void)
{
  s_base_speed = s_saved_base_speed;
  s_obstacle_state = OBSTACLE_IDLE;
  s_obstacle_enter_tick = 0;
  s_obstacle_phase_tick = 0;
  s_obstacle_left_distance = 0;
  s_obstacle_right_distance = 0;
}

static void TraceFollow_UpdateObstacle(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t elapsed = now - s_obstacle_phase_tick;

  switch (s_obstacle_state)
  {
    case OBSTACLE_IDLE:
      break;

    case OBSTACLE_STOPPING:
      TraceFollow_ApplyStop();
      if ((now - s_obstacle_enter_tick) >= TRACE_OBSTACLE_SETTLE_MS)
      {
        s_obstacle_state = OBSTACLE_TURN_LEFT;
        s_obstacle_phase_tick = now;
      }
      break;

    case OBSTACLE_TURN_LEFT:
      TraceFollow_SetServoPulse(s_servo_center);
      TraceFollow_SetMotorDirection(1, 1);
      TraceFollow_SetPwm(10, 280);//遇到障碍时左转向
      if (elapsed >= TRACE_OBSTACLE_SCAN_LEFT_MS)
      {
        s_obstacle_state = OBSTACLE_SAMPLE_LEFT;
        s_obstacle_phase_tick = now;
      }
      break;

    case OBSTACLE_SAMPLE_LEFT:
      TraceFollow_ApplyStop();
      s_obstacle_left_distance = (uint16_t)Ultrasonic_ReadDistanceCm();
      s_obstacle_state = OBSTACLE_TURN_RIGHT;
      s_obstacle_phase_tick = now;
      break;

    case OBSTACLE_TURN_RIGHT:
      TraceFollow_SetServoPulse(s_servo_center);
      TraceFollow_SetMotorDirection(1, 1);
      TraceFollow_SetPwm(220, 10);//遇到障碍时右转向
      if (elapsed >= TRACE_OBSTACLE_SCAN_RIGHT_MS)
      {
        s_obstacle_state = OBSTACLE_SAMPLE_RIGHT;
        s_obstacle_phase_tick = now;
      }
      break;

    case OBSTACLE_SAMPLE_RIGHT:
      TraceFollow_ApplyStop();
      s_obstacle_right_distance = (uint16_t)Ultrasonic_ReadDistanceCm();
      s_obstacle_state = OBSTACLE_DECIDE;
      s_obstacle_phase_tick = now;
      break;

    case OBSTACLE_DECIDE:
      TraceFollow_ApplyStop();
      if (s_obstacle_left_distance >= TRACE_OBSTACLE_STOP_CM &&
          !(s_obstacle_right_distance >= TRACE_OBSTACLE_STOP_CM))
      {
        TraceFollow_SetMotorDirection(1, 1);
        TraceFollow_SetPwm(s_base_speed / 2, s_base_speed);
      }
      else if (s_obstacle_right_distance >= TRACE_OBSTACLE_STOP_CM &&
               !(s_obstacle_left_distance >= TRACE_OBSTACLE_STOP_CM))
      {
        TraceFollow_SetMotorDirection(1, 1);
        TraceFollow_SetPwm(s_base_speed, s_base_speed / 2);
      }
      else if (s_obstacle_left_distance >= TRACE_OBSTACLE_STOP_CM &&
               s_obstacle_left_distance >= s_obstacle_right_distance)
      {
        TraceFollow_SetMotorDirection(1, 1);
        TraceFollow_SetPwm(s_base_speed / 2, s_base_speed);
      }
      else
      {
        TraceFollow_SetMotorDirection(1, 1);
        TraceFollow_SetPwm(s_base_speed, s_base_speed / 2);
      }
      s_obstacle_state = OBSTACLE_FORWARD;
      s_obstacle_phase_tick = now;
      break;

    case OBSTACLE_FORWARD:
      if (elapsed >= TRACE_OBSTACLE_FORWARD_MS)
      {
        s_obstacle_state = OBSTACLE_RESUME;
      }
      break;

    case OBSTACLE_RESUME:
      TraceFollow_ApplyResume();
      break;

    default:
      s_obstacle_state = OBSTACLE_IDLE;
      break;
  }
}

void TraceFollow_Init(void)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
  TraceFollow_SetMotorDirection(1, 1);
  TraceFollow_SetPwm(0, 0);
  TraceFollow_SetServoPulse(s_servo_center);
}

void TraceFollow_Start(void)
{
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_trace_adc_dma, TRACE_ADC_COUNT) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
}

void TraceFollow_Stop(void)
{
  HAL_ADC_Stop_DMA(&hadc1);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  HAL_TIM_Encoder_Stop(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Stop(&htim4, TIM_CHANNEL_ALL);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

void TraceFollow_Update(void)
{
  uint16_t left = 0;
  uint16_t middle = 0;
  uint16_t right = 0;
  float error = 0.0f;

  if (s_obstacle_state != OBSTACLE_IDLE)
  {
    TraceFollow_UpdateObstacle();
    return;
  }

  TraceFollow_ReadAdc(&left, &middle, &right);
  error = TraceFollow_ComputeError(left, middle, right);
  s_current_error = error;

  if (fabsf(error) > TRACE_SERVO_SWITCH_THRESHOLD)
  {
    s_left_target_speed = s_base_speed - (int16_t)(error * 120.0f);
    s_right_target_speed = s_base_speed + (int16_t)(error * 120.0f);
    TraceFollow_UpdateSteering(error);
  }
  else
  {
    s_left_target_speed = s_base_speed;
    s_right_target_speed = s_base_speed;
    TraceFollow_SetServoPulse(s_servo_center);
  }

  TraceFollow_SetMotorDirection(1, 1);
  TraceFollow_SetPwm(s_base_speed, s_base_speed);
  TraceFollow_UpdateSpeedLoop();//读取编码器速度，比较目标速度和实际速度，用PID调整左右电机PWM
}

void TraceFollow_GetAdc(uint16_t *left, uint16_t *middle, uint16_t *right)
{
  TraceFollow_ReadAdc(left, middle, right);
}

float TraceFollow_GetError(void)
{
  return s_current_error;
}

uint16_t TraceFollow_GetServoPulse(void)
{
  return s_servo_current;
}

void TraceFollow_SetSpeed(int16_t base_speed)
{
  s_base_speed = base_speed;
  s_saved_base_speed = base_speed;
}

void TraceFollow_SetPid(float kp, float ki, float kd)
{
  s_trace_pid.kp = kp;
  s_trace_pid.ki = ki;
  s_trace_pid.kd = kd;
}

void TraceFollow_SetServoCenter(uint16_t center)
{
  s_servo_center = center;
}

void TraceFollow_SetServoRange(uint16_t min, uint16_t max)
{
  s_servo_min = min;
  s_servo_max = max;
}

void TraceFollow_SetEncoderPid(float kp, float ki, float kd)
{
  s_left_speed_pid.kp = kp;
  s_left_speed_pid.ki = ki;
  s_left_speed_pid.kd = kd;
  s_right_speed_pid.kp = kp;
  s_right_speed_pid.ki = ki;
  s_right_speed_pid.kd = kd;
}

void TraceFollow_SetEncoderTarget(int16_t left_target, int16_t right_target)
{
  s_left_target_speed = left_target;
  s_right_target_speed = right_target;
}

void Bluetooth_SendTraceState(uint16_t left, uint16_t middle, uint16_t right, float error)
{
  char tx_buf[64];
  snprintf(tx_buf, sizeof(tx_buf), "L:%u,M:%u,R:%u,E:%+.3f\r\n", left, middle, right, error);
  HAL_UART_Transmit(&huart1, (uint8_t *)tx_buf, strlen(tx_buf), 100);
}

void TraceFollow_ManualStop(void)
{
  s_obstacle_state = OBSTACLE_IDLE;
  TraceFollow_SetMotorDirection(1, 1);
  TraceFollow_SetPwm(0, 0);
  TraceFollow_SetServoPulse(s_servo_center);
}

void TraceFollow_ManualForward(int16_t speed)
{
  s_obstacle_state = OBSTACLE_IDLE;
  TraceFollow_SetMotorDirection(1, 1);
  TraceFollow_SetPwm(speed, speed);
  TraceFollow_SetServoPulse(s_servo_center);
}

void TraceFollow_ManualBackward(int16_t speed)
{
  s_obstacle_state = OBSTACLE_IDLE;
  TraceFollow_SetMotorDirection(-1, -1);
  TraceFollow_SetPwm(speed, speed);
  TraceFollow_SetServoPulse(s_servo_center);
}

void TraceFollow_ManualLeft(int16_t speed)
{
  s_obstacle_state = OBSTACLE_IDLE;
  TraceFollow_SetMotorDirection(-1, 1);
  TraceFollow_SetPwm(speed, speed);
  TraceFollow_SetServoPulse(s_servo_center);
}

void TraceFollow_ManualRight(int16_t speed)
{
  s_obstacle_state = OBSTACLE_IDLE;
  TraceFollow_SetMotorDirection(1, -1);
  TraceFollow_SetPwm(speed, speed);
  TraceFollow_SetServoPulse(s_servo_center);
}

void TraceFollow_HandleObstacle(uint32_t distance_cm)
{
  if (s_obstacle_state != OBSTACLE_IDLE)
  {
    return;
  }

  if (distance_cm > 0U && distance_cm < TRACE_OBSTACLE_STOP_CM)
  {
    s_saved_base_speed = s_base_speed;
    Pid_Reset(&s_trace_pid);
    Pid_Reset(&s_left_speed_pid);
    Pid_Reset(&s_right_speed_pid);
    s_obstacle_state = OBSTACLE_STOPPING;
    s_obstacle_enter_tick = HAL_GetTick();
    s_obstacle_phase_tick = s_obstacle_enter_tick;
    s_obstacle_left_distance = 0;
    s_obstacle_right_distance = 0;
    TraceFollow_ApplyStop();
  }
}

uint8_t TraceFollow_IsObstacleActive(void)
{
  return (uint8_t)(s_obstacle_state != OBSTACLE_IDLE);
}

uint8_t TraceFollow_GetObstaclePhase(void)
{
  return (uint8_t)s_obstacle_state;
}
