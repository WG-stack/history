/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    "sensor_sample.h"
  * @brief   Caliper displacement sensor data sampling module header file
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __sensor_sample_H__
#define __sensor_sample_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "data_processing.h"  // 包含传感器数据结构定义

/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define SENSOR_CLK_PIN     GPIO_PIN_12
#define SENSOR_DATA_PIN    GPIO_PIN_11
#define SENSOR_MIRROR_PIN  GPIO_PIN_10
#define SENSOR_GPIO_PORT   GPIOC

#define READ_CLK_HIGH()    ((SENSOR_GPIO_PORT->IDR & SENSOR_CLK_PIN) == SENSOR_CLK_PIN)
#define READ_CLK_LOW()     ((SENSOR_GPIO_PORT->IDR & SENSOR_CLK_PIN) == GPIO_PIN_RESET)
#define READ_DATA_BIT()    ((SENSOR_GPIO_PORT->IDR & SENSOR_DATA_PIN) ? 1 : 0)
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void MX_SENSOR_SAMPLE_Init(void);
void Sensor_Read_24bit_Data(void);

/* USER CODE END Enums_Structs_Defines */

/* USER CODE BEGIN Prototypes */

/* USER CODE BEGIN EF */
/* USER CODE END EF */

#ifdef __cplusplus
}
#endif

#endif /* __sensor_sample_H__*/
/* USER CODE BEGIN 1 */


/* USER CODE END 1 */
