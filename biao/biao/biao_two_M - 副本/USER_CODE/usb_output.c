/**
  ******************************************************************************
  * @file           : usb_output.c
  * @brief          : USB数据输出模块实现
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_output.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// USB CDC发送函数（在usbd_cdc_if.c中定义）
extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t usb_tx_buffer[USB_TX_BUFFER_SIZE];

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化USB输出模块
  * @retval None
  */
void USB_Output_Init(void)
{
    memset(usb_tx_buffer, 0, sizeof(usb_tx_buffer));
    // USB设备已在main.c中初始化
}

/**
  * @brief  通过USB发送数据
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval 0-失败, 1-成功
  * @note   使用CDC虚拟串口发送数据
  */
uint8_t USB_Output_SendData(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0 || len > USB_TX_BUFFER_SIZE) {
        return 0;
    }
    
    // 通过CDC虚拟串口发送数据
    if (CDC_Transmit_FS(data, len) == USBD_OK) {
        return 1;
    }
    
    return 0;
}

/**
  * @brief  通过USB发送字符串
  * @param  str: 字符串指针
  * @retval 0-失败, 1-成功
  */
uint8_t USB_Output_SendString(char *str)
{
    if (str == NULL) {
        return 0;
    }
    
    uint16_t len = strlen(str);
    return USB_Output_SendData((uint8_t*)str, len);
}

/**
  * @brief  通过USB发送格式化字符串
  * @param  format: 格式化字符串
  * @retval 0-失败, 1-成功
  */
uint8_t USB_Output_Printf(const char *format, ...)
{
    va_list args;
    int len;
    
    va_start(args, format);
    len = vsnprintf((char*)usb_tx_buffer, USB_TX_BUFFER_SIZE, format, args);
    va_end(args);
    
    if (len > 0 && len < USB_TX_BUFFER_SIZE) {
        return USB_Output_SendData(usb_tx_buffer, len);
    }
    
    return 0;
}

/**
  * @brief  检查USB是否就绪
  * @retval 0-未就绪, 1-就绪
  */
uint8_t USB_Output_IsReady(void)
{
    // TODO: 检查USB连接状态
    // 可以通过检查USBD状态来判断
    return 1;
}

/* Private functions ---------------------------------------------------------*/
