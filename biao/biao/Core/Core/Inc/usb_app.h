/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usb_app.h
  * @brief   USB应用层头文件
  ******************************************************************************
  * @attention
  *
  * 该文件包含USB检测和处理功能的声明
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __USB_APP_H__
#define __USB_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* USB状态定义 */
typedef enum {
    USB_STATE_DISCONNECTED = 0,   // USB断开连接
    USB_STATE_CONNECTED,          // USB连接
    USB_STATE_SUSPENDED,         // USB挂起
    USB_STATE_RESUMED            // USB恢复
} USB_StateTypeDef;

/* USB模式定义 */
typedef enum {
    USB_MODE_CHARGING_ONLY = 0,   // 仅充电模式
    USB_MODE_DATA_TRANSFER,       // 数据传输模式
    USB_MODE_DEBUG                // 调试模式
} USB_ModeTypeDef;

/* USB初始化 */
void USB_APP_Init(void);

/* USB状态检测 */
USB_StateTypeDef USB_APP_GetState(void);

/* USB模式设置 */
void USB_APP_SetMode(USB_ModeTypeDef mode);

/* USB数据传输 */
void USB_APP_SendData(uint8_t* data, uint16_t length);

/* USB数据接收 */
void USB_APP_DataReceive(uint8_t* data, uint16_t length);

/* USB电源管理 */
void USB_APP_PowerManagement(void);

/* USB连接检测中断回调 */
void USB_APP_ConnectCallback(void);

/* USB断开连接中断回调 */
void USB_APP_DisconnectCallback(void);

/* USB任务处理 */
void USB_APP_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_APP_H__ */
