/**
  ******************************************************************************
  * @file           : usb_cdc_handler.c
  * @brief          : USB CDC接收处理实现
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_cdc_handler.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t usb_rx_buffer[USB_RX_BUFFER_SIZE];
static volatile uint16_t usb_rx_length = 0;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  USB CDC接收回调函数
  * @param  Buf: 接收数据缓冲区
  * @param  Len: 接收数据长度
  * @retval None
  * @note   需要在usbd_cdc_if.c的CDC_Receive_FS函数中调用此函数
  */
void USB_CDC_RxCallback(uint8_t* Buf, uint32_t Len)
{
    if (Len > 0 && Len <= USB_RX_BUFFER_SIZE) {
        // 复制数据到缓冲区
        memcpy(usb_rx_buffer, Buf, Len);
        usb_rx_length = Len;
        
        // TODO: 在这里处理接收到的USB数据
        // 可以解析命令或转发数据
        // 示例：
        // if (usb_rx_length > 0) {
        //     // 处理 usb_rx_buffer 中的数据
        // }
    }
}

/**
  * @brief  获取USB接收数据长度
  * @retval 接收到的数据长度
  */
uint16_t USB_CDC_GetRxDataLength(void)
{
    return usb_rx_length;
}

/**
  * @brief  获取USB接收缓冲区指针
  * @retval 接收缓冲区指针
  */
uint8_t* USB_CDC_GetRxBuffer(void)
{
    return usb_rx_buffer;
}

/* Private functions ---------------------------------------------------------*/
