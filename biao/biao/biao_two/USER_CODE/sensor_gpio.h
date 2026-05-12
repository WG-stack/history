/**
  ******************************************************************************
  * @file           : sensor_gpio.h
  * @brief          : GPIO软件同步串行传感器数据采样模块头文件
  * @author         : 量博科技
  * @date           : 2026-01-15
  ******************************************************************************
  * @attention
  * 
  * 采用GPIO软件同步串行方式替代硬件SPI
  * 硬件接口：
  * - PC12 (CLK)：时钟信号，首次下降沿触发中断
  * - PC11 (DATA)：数据信号，24位串行数据
  * 
  * 采样特点：
  * - 24位数据，LSB先传输
  * - 上升沿采样数据
  * - 数据头校验：首位必须为1
  * - 结束判定：CLK持续高电平500μs
  * - SysTick中断管控：采样期间关闭，避免时序干扰
  * - 超时保护：防止时钟异常导致死循环
  * 
  ******************************************************************************
  */

#ifndef __SENSOR_GPIO_H
#define __SENSOR_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint32_t raw_data;          // 原始24位数据
    float    value;             // 转换后的测量值
    uint8_t  is_negative;       // 符号：0-正数，1-负数
    uint8_t  is_inch;           // 单位：0-公制mm，1-英制inch
    uint8_t  data_valid;        // 数据有效标志
    uint32_t timestamp;         // 时间戳
} SensorData_t;

/* Exported constants --------------------------------------------------------*/
#define SENSOR_DATA_BITS   24        // 数据位数

/* GPIO引脚定义 */
#define SENSOR_GPIO_PORT   GPIOC
#define SENSOR_CLK_PIN     GPIO_PIN_12   // PC12 - 时钟
#define SENSOR_DATA_PIN    GPIO_PIN_11   // PC11 - 数据

/* GPIO读取宏 */
#define READ_CLK_HIGH()    (SENSOR_GPIO_PORT->IDR & SENSOR_CLK_PIN)
#define READ_CLK_LOW()     (!(SENSOR_GPIO_PORT->IDR & SENSOR_CLK_PIN))
#define READ_DATA_BIT()    ((SENSOR_GPIO_PORT->IDR & SENSOR_DATA_PIN) ? 1 : 0)

/* Exported functions prototypes ---------------------------------------------*/
void Sensor_GPIO_Init(void);
void Sensor_GPIO_Read_24bit_Data(void);
uint8_t Sensor_GPIO_GetData(SensorData_t *data);
void Sensor_GPIO_EnableInterrupt(void);
void Sensor_GPIO_DisableInterrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_GPIO_H */
