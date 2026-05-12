/**
  ******************************************************************************
  * @file           : sensor_gpio.c
  * @brief          : GPIO软件同步串行传感器数据采样模块实现
  * @author         : 量博科技
  * @date           : 2026-01-15
  ******************************************************************************
  * @attention
  * 
  * 高速24位数据采样，直接寄存器访问
  * 无硬件SPI，纯GPIO同步串行采样
  * PC12 = CLK（仅第一次下降沿中断触发，后续软件查询）
  * PC11 = DATA
  * 
  * 最后一位判定：CLK拉高后持续500μs不变则认为数据传输结束
  * SysTick中断管控：采样期间关闭，避免时序干扰
  * 引脚配置：PC12/PC11 高速上拉输入，提升高频时钟响应速度
  * 采样时序：标准while电平等待模式，简洁易读
  * 
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "sensor_gpio.h"
#include "tim.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define MAX_DELAY          6000      // 时钟等待超时阈值 
#define END_DELAY_US       500       // 数据结束判定时间(μs)，与参考工程一致
#define CYCLES_PER_US      32        // 32MHz主频：1μs=32个指令周期

/* Private variables ---------------------------------------------------------*/
static SensorData_t sensor_data;
static volatile uint8_t data_ready_flag = 0;
static volatile uint32_t sensor_raw_value = 0;  // 中断中只存原始值，主循环再解析

/* Private function prototypes -----------------------------------------------*/
static void delay_us(uint32_t us);
static void Process_SensorData(uint32_t raw_data);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  传感器GPIO初始化（高速上拉输入配置）
  * @retval None
  */
void Sensor_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能GPIOC时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // 配置PC11：高速上拉输入（数据引脚）
    HAL_GPIO_DeInit(SENSOR_GPIO_PORT, SENSOR_DATA_PIN);
    GPIO_InitStruct.Pin = SENSOR_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SENSOR_GPIO_PORT, &GPIO_InitStruct);
    
    // 配置PC12：高速上拉输入+外部中断下降沿触发（时钟引脚）
    HAL_GPIO_DeInit(SENSOR_GPIO_PORT, SENSOR_CLK_PIN);
    GPIO_InitStruct.Pin = SENSOR_CLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // 下降沿触发中断
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SENSOR_GPIO_PORT, &GPIO_InitStruct);
    
    // 配置NVIC中断优先级
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);  // 最高优先级
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    
    // 初始化数据结构
    memset(&sensor_data, 0, sizeof(SensorData_t));
    data_ready_flag = 0;
}

/**
  * @brief  使能传感器中断
  * @retval None
  */
void Sensor_GPIO_EnableInterrupt(void)
{
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(SENSOR_CLK_PIN);
}

/**
  * @brief  禁用传感器中断
  * @retval None
  */
void Sensor_GPIO_DisableInterrupt(void)
{
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
}

/**
  * @brief  24位传感器数据采样核心函数（软件查询时序+500μs结束判定+SysTick/TIM7管控）
  * @retval None
  */
void Sensor_GPIO_Read_24bit_Data(void)
{
    uint32_t sensor_raw_data = 0;
    uint32_t delay_count = 0;
    int i;
    
    // 采样前关闭SysTick中断和TIM7按键扫描中断，避免干扰采样时序
    HAL_SuspendTick();
    __HAL_TIM_DISABLE_IT(&htim7, TIM_IT_UPDATE);
    data_ready_flag = 0;
    
    // 关闭EXTI中断，避免重复触发
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(SENSOR_CLK_PIN);
    
    // 循环采样24位数据（低位在前，上升沿读数据）
    for(i = 0; i < SENSOR_DATA_BITS; i++)
    {
        // 等待时钟上升沿
        delay_count = 0;
        while(READ_CLK_LOW())
        {
            if(++delay_count > MAX_DELAY)
            {
                HAL_ResumeTick();
                __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
                return;
            }
        }
        
        // 采样数据位并拼接（LSB先传输）
        uint32_t data_bit = READ_DATA_BIT();
        sensor_raw_data |= (data_bit << i);
        
        // 前23位：等待时钟下降沿
        if(i < SENSOR_DATA_BITS - 1)
        {
            delay_count = 0;
            while(READ_CLK_HIGH())
            {
                if(++delay_count > MAX_DELAY)
                {
                    HAL_ResumeTick();
                    __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
                    return;
                }
            }
        }
        // 最后一位：检测时钟是否持续拉高500μs
        else
        {
            // 延时500μs
            delay_us(END_DELAY_US);
            
            // 延时后仍为高电平 → 判定数据结束
            if(!READ_CLK_HIGH())
            {
                HAL_ResumeTick();  // 未满足500μs高电平，采样失败
                __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
                return;
            }
        }
    }
    
    // 中断中只保存原始值，不做浮点运算，缩短中断执行时间
    sensor_raw_value = sensor_raw_data;
    data_ready_flag = 1;
    
    // 恢复SysTick中断和TIM7按键扫描中断
    HAL_ResumeTick();
    __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
}

/**
  * @brief  获取传感器数据
  * @param  data: 数据结构指针
  * @retval 0-无新数据, 1-有新数据
  */
uint8_t Sensor_GPIO_GetData(SensorData_t *data)
{
    if (data_ready_flag && data != NULL) {
        // 临界区保护：读取原始值时短暂关中断
        __disable_irq();
        uint32_t raw = sensor_raw_value;
        data_ready_flag = 0;
        __enable_irq();
        
        // 在主循环上下文中做BCD解码和浮点运算
        Process_SensorData(raw);
        memcpy(data, &sensor_data, sizeof(SensorData_t));
        return 1;
    }
    return 0;
}

/* Private functions ---------------------------------------------------------*/

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
  * @brief  处理传感器原始数据
  * @param  raw_data: 24位原始数据
  * @retval None
  * @note   数据格式：6个nib（半字节），LSB先传输
  *         Nib0~4: 数据位（20位）
  *         Nib5: bit0=符号(0正1负), bit3=单位(0公制1英制)
  */
static void Process_SensorData(uint32_t raw_data)
{
    // 提取各个nib（每个nib 4位）
    uint8_t nib0 = (raw_data >> 0) & 0x0F;   // bit 0-3
    uint8_t nib1 = (raw_data >> 4) & 0x0F;   // bit 4-7
    uint8_t nib2 = (raw_data >> 8) & 0x0F;   // bit 8-11
    uint8_t nib3 = (raw_data >> 12) & 0x0F;  // bit 12-15
    uint8_t nib4 = (raw_data >> 16) & 0x0F;  // bit 16-19
    uint8_t nib5 = (raw_data >> 20) & 0x0F;  // bit 20-23
    
    // 组合数据位（nib0~nib4，共20位）
    uint32_t value_data = 0;
    value_data = nib0;                    // bit 0-3
    value_data |= ((uint32_t)nib1 << 4);  // bit 4-7
    value_data |= ((uint32_t)nib2 << 8);  // bit 8-11
    value_data |= ((uint32_t)nib3 << 12); // bit 12-15
    value_data |= ((uint32_t)nib4 << 16); // bit 16-19
    
    // 提取符号和单位
    uint8_t is_negative = nib5 & 0x01;        // bit0: 符号
    uint8_t is_inch = (nib5 >> 3) & 0x01;     // bit3: 单位
    
    // 保存数据
    sensor_data.raw_data = raw_data;
    sensor_data.is_negative = is_negative;
    sensor_data.is_inch = is_inch;
    
    // 转换为实际测量值
    float value = (float)value_data;
    
    if (is_inch) {
        // 英制单位，通常分辨率为0.0001 inch
        value = value * 0.0001f;
    } else {
        // 公制单位，通常分辨率为0.01 mm
        value = value * 0.01f;
    }
    
    // 应用符号
    if (is_negative) {
        value = -value;
    }
    
    sensor_data.value = value;
    sensor_data.data_valid = 1;
    sensor_data.timestamp = HAL_GetTick();
}
