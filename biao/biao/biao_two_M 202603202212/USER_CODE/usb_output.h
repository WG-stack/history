/**
  ******************************************************************************
  * @file           : usb_output.h
  * @brief          : USB数据输出模块头文件
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

#ifndef __USB_OUTPUT_H
#define __USB_OUTPUT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define USB_TX_BUFFER_SIZE  256

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void USB_Output_Init(void);
uint8_t USB_Output_SendData(uint8_t *data, uint16_t len);
uint8_t USB_Output_SendString(char *str);
uint8_t USB_Output_Printf(const char *format, ...);
uint8_t USB_Output_SendMeasure(float value, uint8_t unit, uint8_t resolution);
uint8_t USB_Output_IsReady(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_OUTPUT_H */
