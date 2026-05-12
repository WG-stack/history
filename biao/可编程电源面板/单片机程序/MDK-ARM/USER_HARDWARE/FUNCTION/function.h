#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "main.h"
#include "rx8025t.h"
#include "at24c64.h"
#include "bcd_output.h"
#include "ad_da.h"
#include "hmi_usart.h"
#include "fiber_485_usart.h"
#include "rs485_usart.h"
#include "cmd_input.h"
#include <string.h>

/**
 * @brief 主功能函数，在main循环中调用
 */
void MainFunction(void);

#endif

