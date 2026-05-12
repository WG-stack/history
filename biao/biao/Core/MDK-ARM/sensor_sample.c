/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    spi.c
  * @brief   SPI communication module using direct register access for high-speed sampling
  ******************************************************************************
  * @attention
  *
  * This module implements high-speed SPI-like data sampling using direct register
  * access to achieve faster performance compared to HAL library functions.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "sensor_sample.h"
#include "main.h"
#include "stm32l1xx_hal.h"
#include "tim.h"
#include "data_processing.h"  // 添加数据处理头文件，以访问parse_sensor_data函数

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 寄存器直接读取宏
#define READ_PC12_HIGH()  ((GPIOC->IDR & GPIO_PIN_12) == GPIO_PIN_12)
#define READ_PC12_LOW()   ((GPIOC->IDR & GPIO_PIN_12) == GPIO_PIN_RESET)
#define READ_PC11_BIT()   ((GPIOC->IDR & GPIO_PIN_11) ? 1 : 0)
// 新增：SPI采样超时计数值宏定义(10μs/单位，原200值)，方便校准
#define SPI_SAMPLE_TIMEOUT_CNT    200
// 内存屏障宏，防止编译器优化寄存器操作
#define __DMB()  __ASM volatile ("dmb" ::: "memory")

/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private variables ---------------------------------------------------------*/
static uint8_t tim7_initialized = 0;  // TIM7初始化标志

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static void spi_interface_gpio_init(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  Initialize the SPI interface with PC12 (clock) and PC11 (data)
  * @retval None
  */
void MX_CUSTOM_SPI_Init(void)
{
  /* Initialize GPIO for SPI interface */
  spi_interface_gpio_init();
}

/**
 * @brief  Initialize GPIO for SPI interface (PC12 and PC11)
 * @retval None
 */
static void spi_interface_gpio_init(void)
{
GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Enable GPIOC clock */
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* Configure PC12 as input with pull-up for clock signal */
  GPIO_InitStruct.Pin = GPIO_PIN_12;  // Clock pin
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Configure PC11 as input with pull-up for data signal */
  GPIO_InitStruct.Pin = GPIO_PIN_11;  // Data pin
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Configure PC10 as output for data mirroring - moved to data_processing.c */
  /* Initialize PC10 to high when no signal - moved to data_processing.c */
}

/**
 * @brief  Read 24-bit data from synchronous serial interface using direct register access
 * @retval 24-bit data value
 */
uint32_t Read_24bit_Data(void)
{
   uint32_t data_temp = 0; // 初始化24位数据缓存

   // 初始化TIM7（仅在首次调用时）
   if (!tim7_initialized) {
       __HAL_RCC_TIM7_CLK_ENABLE();  // 使能TIM7时钟
       TIM7->ARR = 100;              // 设置自动重载值为100 (对应约1ms超时 @ 100kHz)
       TIM7->PSC = 319;              // 预分频器设置为319，使定时器时钟为100kHz (32MHz/(319+1)=100kHz)
       TIM7->SR &= ~TIM_SR_UIF;      // 清除更新中断标志
       tim7_initialized = 1;         // 标记TIM7已初始化
   }

   // 启动TIM7作为超时计时器
   TIM7->CNT = 0;                  // 重置计数器
   TIM7->CR1 |= TIM_CR1_CEN;       // 使能计数器
   __DMB();                        // 内存屏障，确保寄存器操作完成

   for(int i = 0; i < 24; i++)  // 优化：改为 < 24 而不是 <= 23
   {
       // 等待PC12变为高电平（同步触发采样）
       uint32_t timeout_counter = TIM7->CNT;
       while(READ_PC12_LOW()) {
           if((TIM7->CNT - timeout_counter) > SPI_SAMPLE_TIMEOUT_CNT) {  // 宏定义超时值
               goto read_error;  // 快速跳转处理错误
           }
       }
       __DMB();                    // 内存屏障，保证电平检测完成

       // 采样PC11的电平，拼接到data_temp
       data_temp <<= 1;
       uint32_t data_bit = READ_PC11_BIT();
       data_temp |= data_bit;
       
       // 将PC11的值赋给PC10（同相输出）
       if(data_bit) {
           GPIOC->BSRR = GPIO_BSRR_BS_10;  // 设置PC10为高电平
       } else {
           GPIOC->BSRR = GPIO_BSRR_BR_10;  // 设置PC10为低电平
       }
       __DMB();                    // 内存屏障，确保PC10输出完成

       // 等待PC12回落低电平，适配下一个时钟脉冲
       timeout_counter = TIM7->CNT;  // 重置超时计数器
       while(READ_PC12_HIGH()&&i<23) {
           if((TIM7->CNT - timeout_counter) > SPI_SAMPLE_TIMEOUT_CNT) {  // 宏定义超时值
               goto read_error;  // 快速跳转处理错误
           }
       }
       __DMB();                    // 内存屏障，保证电平检测完成
   }
   
   // 成功读取数据，禁用计数器+关闭时钟，降低功耗
   TIM7->CR1 &= ~TIM_CR1_CEN;
   __HAL_RCC_TIM7_CLK_DISABLE();
   __DMB();
   tim7_initialized = 0;        // 重置初始化标志，下次采样重新初始化
   return data_temp;
   
read_error:
   // 发生错误，禁用计数器+关闭时钟，降低功耗
   TIM7->CR1 &= ~TIM_CR1_CEN;
   __HAL_RCC_TIM7_CLK_DISABLE();
   __DMB();
   tim7_initialized = 0;        // 重置初始化标志，下次采样重新初始化
   return 0; // 返回无效数据
}

/* USER CODE END 1 */