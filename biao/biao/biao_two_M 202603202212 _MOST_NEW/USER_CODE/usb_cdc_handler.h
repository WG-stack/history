/**
  ******************************************************************************
  * @file           : usb_cdc_handler.h
  * @brief          : USB CDC接收处理头文件
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  * @note
  * 
  * 本文件提供USB CDC接收数据的处理接口
  * 需要在usbd_cdc_if.c的CDC_Receive_FS函数中调用USB_CDC_RxCallback
  * 
  ******************************************************************************
  */

#ifndef __USB_CDC_HANDLER_H
#define __USB_CDC_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
#define USB_RX_BUFFER_SIZE  256

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void USB_CDC_RxCallback(uint8_t* Buf, uint32_t Len);
uint16_t USB_CDC_GetRxDataLength(void);
uint8_t* USB_CDC_GetRxBuffer(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CDC_HANDLER_H */
