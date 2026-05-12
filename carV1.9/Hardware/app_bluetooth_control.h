#ifndef __APP_BLUETOOTH_CONTROL_H
#define __APP_BLUETOOTH_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

typedef enum
{
  APP_BT_MODE_AUTO = 0,
  APP_BT_MODE_MANUAL = 1
} AppBtMode_t;

typedef enum
{
  APP_BT_CMD_NONE = 0,
  APP_BT_CMD_FORWARD,
  APP_BT_CMD_BACKWARD,
  APP_BT_CMD_LEFT,
  APP_BT_CMD_RIGHT,
  APP_BT_CMD_STOP,
  APP_BT_CMD_AUTO,
  APP_BT_CMD_MANUAL
} AppBtCommand_t;

void AppBt_Init(void);
void AppBt_Reset(void);
void AppBt_PushByte(uint8_t byte);
void AppBt_Process(void);

void AppBt_SetEchoDebug(uint8_t enable);
uint8_t AppBt_IsEchoDebugEnabled(void);
void AppBt_SetMode(AppBtMode_t mode);

AppBtMode_t AppBt_GetMode(void);
AppBtCommand_t AppBt_GetLastCommand(void);

void AppBt_SendText(const char *text);
void AppBt_SendMode(void);
void AppBt_SendAck(AppBtCommand_t cmd);
void AppBt_SendRawByte(uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif
