/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    sensor_sample.c
  * @brief   Caliper displacement sensor data sampling module (GPIO sync serial)
  ******************************************************************************
  * @attention
  *
  * High-speed 24-bit data sampling with direct register access,
  * TIM7 timeout protection and low power consumption optimization for STM32L152.
  * No hardware SPI used, pure GPIO synchronous serial sampling.
  * PC12 = CLK（仅第一次下降沿中断触发，后续软件查询），PC11 = DATA
  * 最后一位判定：CLK拉高后持续100μs不变则认为数据传输结束
  * SysTick中断管控：采样期间关闭，避免时序干扰
  * 引脚配置：PC12/PC11 高速上拉输入，提升高频时钟响应速度
  * 采样时序：标准while电平等待模式，简洁易读
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "sensor_sample.h"
#include "main.h"
#include "stm32l1xx_hal.h"
#include "tim.h"
#include "data_processing.h"
#include "global_variables.h"
#include "cmsis_armcc.h"
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */
/* Private defines -----------------------------------------------------------*/
#define SENSOR_DATA_BITS   24        // 数据位数
#define MAX_DELAY          5000      // 时钟等待超时阈值
#define END_DELAY_US       500       // 数据结束判定时间(μs)
#define CYCLES_PER_US      16        // 16MHz主频：1μs=16个指令周期
/* USER CODE END PD */
/* Private variables ---------------------------------------------------------*/
// 无需跟踪中断触发状态，直接通过开关中断控制
static volatile uint32_t* sensor_mirror_port = &(SENSOR_GPIO_PORT->BSRR);
static uint32_t sensor_mirror_pin_set = SENSOR_MIRROR_PIN;
static uint32_t sensor_mirror_pin_reset = (uint32_t)SENSOR_MIRROR_PIN << 16U;
/* USER CODE BEGIN PV */
/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
static void sensor_gpio_init(void);
static void delay_us(uint32_t us);              // 精准us延时函数
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
/**
  * @brief  传感器采样接口初始化
  * @retval None
  */
void MX_SENSOR_SAMPLE_Init(void)
{
  sensor_gpio_init();
  // 中断已在gpio.c中配置，这里不再重复设置
}

/**
  * @brief  传感器GPIO初始化（高速上拉输入配置）
  * @retval None
  */
static void sensor_gpio_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  // 配置PC11：高速上拉输入（数据引脚）
  HAL_GPIO_DeInit(SENSOR_GPIO_PORT, SENSOR_DATA_PIN);
  GPIO_InitStruct.Pin = SENSOR_DATA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SENSOR_GPIO_PORT, &GPIO_InitStruct);
  
  // 配置PC10：推挽输出（镜像输出，可选）
  HAL_GPIO_DeInit(SENSOR_GPIO_PORT, SENSOR_MIRROR_PIN);
  GPIO_InitStruct.Pin = SENSOR_MIRROR_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SENSOR_GPIO_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(SENSOR_GPIO_PORT, SENSOR_MIRROR_PIN, GPIO_PIN_RESET);
}

// 直接使用HAL库函数控制中断
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  精准微秒延时函数（基于指令周期）
  * @param  us: 延时微秒数
  * @retval None
  */
static void delay_us(uint32_t us)
{
  us *= CYCLES_PER_US;
  while(us--);
}

/**
  * @brief  24位传感器数据采样核心函数（软件同步串行）
  * @retval 采样得到的24位数据
  */
 /**
  * @brief  24位数据采样核心函数（软件查询时序+100μs结束判定+SysTick管控）
  * @retval None
  */
void Sensor_Read_24bit_Data(void)
{
  uint32_t sensor_data = 0;
  uint32_t delay_count = 0;

   // 采样前关闭SysTick中断，避免干扰采样时序
   HAL_SuspendTick();
	LL_TIM_DisableIT_UPDATE(TIM7);
    sensor_data_ready_flag = 0;
  // 关闭中断，避免重复触发
  HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);  // 直接使用HAL库函数
  __HAL_GPIO_EXTI_CLEAR_IT(SENSOR_CLK_PIN);
  
  // 保存中断状态，以便在采样完成后重新启用中断

  // 循环采样24位数据（低位在前，上升沿读数据）
 { for(int i = 0; i < SENSOR_DATA_BITS; i++)
  {
    // 等待时钟上升沿
    delay_count = 0;
    while(READ_CLK_LOW())
    {
      if(++delay_count > MAX_DELAY)
      {
       // current_sample.value =0;
        HAL_ResumeTick(); // 恢复SysTick再返回
        return;
      }
    }
    // 数据稳定延时：4个NOP=0.25μs(满足<1μs要求)
  
     //__NOP();__NOP();__NOP();__NOP();__NOP();
    // 采样数据位并拼接
    uint32_t data_bit = READ_DATA_BIT();
    sensor_data |= (data_bit << i);

    // 镜像输出到PC10
 //  *sensor_mirror_port = data_bit ? sensor_mirror_pin_set : sensor_mirror_pin_reset;

    // 前23位：等待时钟下降沿
    if(i < SENSOR_DATA_BITS - 1)
    {
      delay_count = 0;
      while(READ_CLK_HIGH())
      {
        if(++delay_count > MAX_DELAY)
        {
         // current_sample.value =0;
          HAL_ResumeTick(); // 恢复SysTick再返回
          return;
        }
      }
      // 时序稳定延时：2个NOP=0.125μs
     // __NOP();__NOP();__NOP();__NOP();__NOP();
    }
    // 最后一位：检测时钟是否持续拉高100μs
    else
    {
      // 延时100μs
      delay_us(END_DELAY_US);
      // 延时后仍为高电平 → 判定数据结束
      if(!READ_CLK_HIGH())
      {
        //current_sample.value = 0; // 未满足100μs高电平，采样失败
        HAL_ResumeTick(); // 恢复SysTick再返回
        return;
      }
      
     }
  }

  // 将采集到的数据赋值给全局变量
  current_sample.value = sensor_data;
  sensor_data_ready_flag = 1;//数据好
  // 恢复SysTick中断
  HAL_ResumeTick();
  LL_TIM_EnableIT_UPDATE(TIM7);
  // 采样完成后重新启用中断，以允许下次采样
  // 确保中断被正确启用
 // HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
}
