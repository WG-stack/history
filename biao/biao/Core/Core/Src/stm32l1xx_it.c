/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32l1xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l1xx_it.h"
#include "rtc.h"
#include "stm32l1xx_hal_rtc_ex.h"
#include "sensor_sample.h"
#include "global_variables.h"
#include "tim.h"  // 直接包含tim.h以确保访问htim7

/* USER CODE BEGIN Includes */
#include "sensor_sample.h"
/* USER CODE END Includes */
/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */
/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
/* USER CODE BEGIN EV */
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */
  /* USER CODE END NonMaskableInt_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN W1_HardFault_IRQn 0 */
  /* USER CODE END W1_HardFault_IRQn 0 */
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */
  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
  /* USER CODE END W1_MemoryManagement_IRQn 0 */
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */
  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN W1_BusFault_IRQn 0 */
  /* USER CODE END W1_BusFault_IRQn 0 */
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */
  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
  }
  /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
  /* USER CODE END W1_UsageFault_IRQn 0 */
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVC_IRQn 0 */
  /* USER CODE END SVC_IRQn 0 */
  /* USER CODE BEGIN SVC_IRQn 1 */
  /* USER CODE END SVC_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */
  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */
  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */
  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */
  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */
  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */
  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32L1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32l1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel4_IRQn 0 */
  /* USER CODE END DMA1_Channel4_IRQn 0 */
  /* USER CODE BEGIN DMA1_Channel4_IRQn 1 */
  /* USER CODE END DMA1_Channel4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel5_IRQn 0 */
  /* USER CODE END DMA1_Channel5_IRQn 0 */
  /* USER CODE BEGIN DMA1_Channel5_IRQn 1 */
  /* USER CODE END DMA1_Channel5_IRQn 1 */
}

/* USER CODE BEGIN 1 */
/**
  * @brief  This function handles external line 15-10 interrupt request.
  * @param  None
  * @retval None
  */
void EXTI15_10_IRQHandler(void)
{
 /* USER CODE BEGIN EXTI15_10_IRQn 0 */
  /* USER CODE END EXTI15_10_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);  // 处理PC12引脚的外部中断
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */
  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
  * @brief  Callback function for GPIO EXTI interrupt.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* USER CODE BEGIN HAL_GPIO_EXTI_Callback */
  if (GPIO_Pin == GPIO_PIN_12)  // PC12下降沿触发首次采样
  {
    // 下降沿中断发生时检查数据位是否是1，是就采样，不是则退出
    if (READ_DATA_BIT() == 1)  // 检查PC11数据位是否为1
    {
      // 添加调试标记，表明中断被触发
    //  extern volatile uint8_t interrupt_debug_flag;
     // interrupt_debug_flag = 1;
      
      extern void Sensor_Read_24bit_Data(void);
      // 临时禁用定时器中断，防止在传感器数据处理过程中发生中断冲突
      LL_TIM_DisableIT_UPDATE(TIM7);
      
      Sensor_Read_24bit_Data();
      
      // 重新启用定时器中断
      LL_TIM_EnableIT_UPDATE(TIM7);
      // 中断重使能在Sensor_Read_24bit_Data函数内完成
    }
    // 如果数据位不是1，则直接退出，不进行采样
  }
  /* USER CODE END HAL_GPIO_EXTI_Callback */
}

/**
  * @brief  This function handles RTC Wake-up interrupt request.
  * @param  None
  * @retval None
  */
void RTC_WKUP_IRQHandler(void)
{
  /* USER CODE BEGIN RTC_WKUP_IRQn 0 */
  HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
  /* USER CODE END RTC_WKUP_IRQn 0 */
}

/**
  * @brief  Wake-up timer callback function when exiting STOP mode
  * @param  hrtc: RTC handle
  * @retval None
  */
void HAL_RTCEx_WakeUpTimerCallback(RTC_HandleTypeDef *hrtc)
{
  /* USER CODE BEGIN RTCEx_WakeUpTimerCallback */
  // After waking up from STOP mode, the system clock configuration is handled in main loop
  // The main loop will call SystemClock_ConfigAfterWakeupFromStop() after returning from WFI
  // This ensures consistent clock reconfiguration after every STOP mode exit
  /* USER CODE END RTCEx_WakeUpTimerCallback */
}

/**
 * @brief  This function handles TIM7 global interrupt request.
 * @param  None
 * @retval None
 */
void TIM7_IRQHandler(void)
{
  /* USER CODE BEGIN TIM7_IRQn 0 */
  /* USER CODE END TIM7_IRQn 0 */
  
  // 检查是否是更新中断
  if(LL_TIM_IsActiveFlag_UPDATE(TIM7) == 1)
  {
    LL_TIM_ClearFlag_UPDATE(TIM7);  // 清除更新中断
    
    // 增加中断计数器（带保护）
    extern uint32_t tim7_interrupt_count;
    tim7_interrupt_count++;
    
    // 设置按键扫描标志，主循环中处理具体逻辑
    extern volatile uint8_t key_scan_pending;
    key_scan_pending = 1;
    HAL_SuspendTick();
    // 直接调用按键扫描函数，而不是通过HAL回调
    extern void KeyDriver_Timer_Scan(void);
    KeyDriver_Timer_Scan();
		HAL_ResumeTick();
  }
  /* USER CODE BEGIN TIM7_IRQn 1 */
  /* USER CODE END TIM7_IRQn 1 */
}
