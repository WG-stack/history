/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32l1xx_ll_gpio.h"
#include "stm32l1xx_ll_bus.h"
#include "stm32l1xx_ll_tim.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define K_P0_Pin GPIO_PIN_13
#define K_P0_GPIO_Port GPIOC
#define K_P1_Pin GPIO_PIN_14
#define K_P1_GPIO_Port GPIOC
#define K_P2_Pin GPIO_PIN_15
#define K_P2_GPIO_Port GPIOC
#define S_INMM_Pin GPIO_PIN_4
#define S_INMM_GPIO_Port GPIOA
#define BAT_V_Pin GPIO_PIN_5
#define BAT_V_GPIO_Port GPIOA
#define S_ZERO_Pin GPIO_PIN_2
#define S_ZERO_GPIO_Port GPIOB
#define S_POWER_Pin GPIO_PIN_9
#define S_POWER_GPIO_Port GPIOC
#define W_POWER_Pin GPIO_PIN_2
#define W_POWER_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
