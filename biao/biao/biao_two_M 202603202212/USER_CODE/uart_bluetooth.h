/**
  ******************************************************************************
  * @file           : uart_bluetooth.h
  * @brief          : 串口蓝牙输出模块头文件
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

#ifndef __UART_BLUETOOTH_H
#define __UART_BLUETOOTH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define UART_TX_BUFFER_SIZE  256
#define UART_RX_BUFFER_SIZE  256

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void UART_BT_Init(void);
uint8_t UART_BT_SendData(uint8_t *data, uint16_t len);
uint8_t UART_BT_SendString(char *str);
uint8_t UART_BT_Printf(const char *format, ...);
uint8_t UART_BT_SendMeasure(float value, uint8_t unit, uint8_t resolution);
void UART_BT_IdleCallback(void);
uint16_t UART_BT_GetRxDataLength(void);
uint8_t* UART_BT_GetRxBuffer(void);
void UART_BT_IdleInterruptCallback(void);
uint8_t* UART_BT_GetReceivedData(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_BLUETOOTH_H */
