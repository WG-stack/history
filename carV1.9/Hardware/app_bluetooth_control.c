#include "app_bluetooth_control.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

#define APP_BT_RX_LINE_MAX 32U

static volatile AppBtMode_t s_mode = APP_BT_MODE_AUTO;
static volatile AppBtCommand_t s_last_cmd = APP_BT_CMD_NONE;
static volatile uint8_t s_echo_debug = 0U;
static char s_rx_line[APP_BT_RX_LINE_MAX];
static uint8_t s_rx_index = 0;

static AppBtCommand_t AppBt_ParseCommand(const char *cmd)
{
  if (cmd == NULL || cmd[0] == '\0')
  {
    return APP_BT_CMD_NONE;
  }

  if (strcmp(cmd, "F") == 0 || strcmp(cmd, "FORWARD") == 0)
  {
    return APP_BT_CMD_FORWARD;
  }
  if (strcmp(cmd, "B") == 0 || strcmp(cmd, "BACK") == 0)
  {
    return APP_BT_CMD_BACKWARD;
  }
  if (strcmp(cmd, "L") == 0 || strcmp(cmd, "LEFT") == 0)
  {
    return APP_BT_CMD_LEFT;
  }
  if (strcmp(cmd, "R") == 0 || strcmp(cmd, "RIGHT") == 0)
  {
    return APP_BT_CMD_RIGHT;
  }
  if (strcmp(cmd, "S") == 0 || strcmp(cmd, "STOP") == 0)
  {
    return APP_BT_CMD_STOP;
  }
  if (strcmp(cmd, "A") == 0 || strcmp(cmd, "AUTO") == 0)
  {
    return APP_BT_CMD_AUTO;
  }
  if (strcmp(cmd, "M") == 0 || strcmp(cmd, "MANUAL") == 0)
  {
    return APP_BT_CMD_MANUAL;
  }

  return APP_BT_CMD_NONE;
}

static void AppBt_ApplyCommand(AppBtCommand_t cmd)
{
  switch (cmd)
  {
    case APP_BT_CMD_AUTO:
      s_mode = APP_BT_MODE_AUTO;
      break;
    case APP_BT_CMD_MANUAL:
      s_mode = APP_BT_MODE_MANUAL;
      break;
    default:
      break;
  }

  if (cmd != APP_BT_CMD_NONE)
  {
    s_last_cmd = cmd;
  }
}

void AppBt_Init(void)
{
  AppBt_Reset();
}

void AppBt_Reset(void)
{
  s_mode = APP_BT_MODE_MANUAL;
  s_last_cmd = APP_BT_CMD_NONE;
  s_rx_index = 0;
  s_echo_debug = 0U;
  memset(s_rx_line, 0, sizeof(s_rx_line));
}

void AppBt_PushByte(uint8_t byte)
{
  char debug_buf[32];
  AppBtCommand_t cmd;
  char c = (char)byte;

  if (s_echo_debug != 0U)
  {
    snprintf(debug_buf, sizeof(debug_buf), "RX:%c\r\n", (byte >= 32U && byte <= 126U) ? c : '.');
    AppBt_SendText(debug_buf);
  }

  if (byte == '\r')
  {
    return;
  }

  if (byte == '\n')
  {
    s_rx_line[s_rx_index] = '\0';
    cmd = AppBt_ParseCommand(s_rx_line);

    if (s_echo_debug != 0U)
    {
      snprintf(debug_buf, sizeof(debug_buf), "CMD:%s\r\n", s_rx_line);
      AppBt_SendText(debug_buf);
    }

    AppBt_ApplyCommand(cmd);
    s_rx_index = 0;
    memset(s_rx_line, 0, sizeof(s_rx_line));
    return;
  }

  if (s_rx_index < (APP_BT_RX_LINE_MAX - 1U))
  {
    s_rx_line[s_rx_index++] = c;
  }
  else
  {
    s_rx_index = 0;
    memset(s_rx_line, 0, sizeof(s_rx_line));
  }

  if (s_rx_index == 1U)
  {
    cmd = AppBt_ParseCommand(s_rx_line);
    if (cmd != APP_BT_CMD_NONE)
    {
      AppBt_ApplyCommand(cmd);
      if (s_echo_debug != 0U)
      {
        snprintf(debug_buf, sizeof(debug_buf), "CMD:%s\r\n", s_rx_line);
        AppBt_SendText(debug_buf);
      }
      s_rx_index = 0;
      memset(s_rx_line, 0, sizeof(s_rx_line));
    }
  }
}

void AppBt_Process(void)
{
  /* 预留给主循环使用，后续可扩展为处理超时/状态上报 */
}

void AppBt_SetEchoDebug(uint8_t enable)
{
  s_echo_debug = (enable != 0U) ? 1U : 0U;
}

uint8_t AppBt_IsEchoDebugEnabled(void)
{
  return s_echo_debug;
}

void AppBt_SetMode(AppBtMode_t mode)
{
  if (mode == APP_BT_MODE_AUTO || mode == APP_BT_MODE_MANUAL)
  {
    s_mode = mode;
  }
}

AppBtMode_t AppBt_GetMode(void)
{
  return s_mode;
}

AppBtCommand_t AppBt_GetLastCommand(void)
{
  return s_last_cmd;
}

void AppBt_SendText(const char *text)
{
  if (text == NULL)
  {
    return;
  }
  HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)strlen(text), 100);
}

void AppBt_SendRawByte(uint8_t byte)
{
  HAL_UART_Transmit(&huart1, &byte, 1, 100);
}

void AppBt_SendMode(void)
{
  const char *msg = (s_mode == APP_BT_MODE_AUTO) ? "MODE:AUTO\r\n" : "MODE:MANUAL\r\n";
  AppBt_SendText(msg);
}

void AppBt_SendAck(AppBtCommand_t cmd)
{
  const char *msg = "ACK:UNKNOWN\r\n";

  switch (cmd)
  {
    case APP_BT_CMD_FORWARD: msg = "ACK:F\r\n"; break;
    case APP_BT_CMD_BACKWARD: msg = "ACK:B\r\n"; break;
    case APP_BT_CMD_LEFT: msg = "ACK:L\r\n"; break;
    case APP_BT_CMD_RIGHT: msg = "ACK:R\r\n"; break;
    case APP_BT_CMD_STOP: msg = "ACK:S\r\n"; break;
    case APP_BT_CMD_AUTO: msg = "ACK:AUTO\r\n"; break;
    case APP_BT_CMD_MANUAL: msg = "ACK:MANUAL\r\n"; break;
    default: break;
  }

  AppBt_SendText(msg);
}
