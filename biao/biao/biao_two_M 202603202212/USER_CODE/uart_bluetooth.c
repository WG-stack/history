/**
  ******************************************************************************
  * @file           : uart_bluetooth.c
  * @brief          : 串口蓝牙输出模块实现
  * @author         : 量博科技
  * @date           : 2025-12-30
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "uart_bluetooth.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t uart_tx_buffer[UART_TX_BUFFER_SIZE];
static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t tx_busy = 0;
static volatile uint16_t rx_data_length = 0;

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化串口蓝牙模块
  * @retval None
  */
void UART_BT_Init(void)
{
    memset(uart_tx_buffer, 0, sizeof(uart_tx_buffer));
    memset(uart_rx_buffer, 0, sizeof(uart_rx_buffer));
    tx_busy = 0;
    rx_data_length = 0;
    
    // 使能串口空闲中断
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
    
    // 启动DMA接收
    HAL_UART_Receive_DMA(&huart1, uart_rx_buffer, UART_RX_BUFFER_SIZE);
}

/**
  * @brief  通过串口发送数据
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval 0-失败, 1-成功
  */
uint8_t UART_BT_SendData(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0 || len > UART_TX_BUFFER_SIZE) {
        return 0;
    }
    
    // 等待上一次发送完成
    uint32_t timeout = HAL_GetTick() + 100;
    while (tx_busy && HAL_GetTick() < timeout) {
        // 等待
    }
    
    if (tx_busy) {
        return 0;  // 超时
    }
    
    tx_busy = 1;
    
    // 通过DMA发送数据
    if (HAL_UART_Transmit_DMA(&huart1, data, len) == HAL_OK) {
        return 1;
    }
    
    tx_busy = 0;
    return 0;
}

/**
  * @brief  通过串口发送字符串
  * @param  str: 字符串指针
  * @retval 0-失败, 1-成功
  */
uint8_t UART_BT_SendString(char *str)
{
    if (str == NULL) {
        return 0;
    }
    
    uint16_t len = strlen(str);
    return UART_BT_SendData((uint8_t*)str, len);
}

/**
  * @brief  通过串口发送格式化字符串
  * @param  format: 格式化字符串
  * @retval 0-失败, 1-成功
  */
uint8_t UART_BT_Printf(const char *format, ...)
{
    va_list args;
    int len;
    
    va_start(args, format);
    len = vsnprintf((char*)uart_tx_buffer, UART_TX_BUFFER_SIZE, format, args);
    va_end(args);
    
    if (len > 0 && len < UART_TX_BUFFER_SIZE) {
        return UART_BT_SendData(uart_tx_buffer, len);
    }
    
    return 0;
}

/**
  * @brief  通过蓝牙串口发送测量数据
  * @param  value: 显示值
  * @param  unit: 单位 (0=mm, 1=inch)
  * @param  resolution: 分辨率 (0=0.0005mm, 1=0.001mm, 2=0.01mm)
  * @retval 0-失败, 1-成功
  */
uint8_t UART_BT_SendMeasure(float value, uint8_t unit, uint8_t resolution)
{
    int decimal;
    const char *unit_str;

    if (unit == 0) {
        unit_str = "mm";
        switch (resolution) {
            case 0:  decimal = 4; break;
            case 1:  decimal = 3; break;
            default: decimal = 2; break;
        }
    } else {
        unit_str = "inch";
        switch (resolution) {
            case 2:  decimal = 4; break;
            default: decimal = 5; break;
        }
    }

    return UART_BT_Printf("%.*f %s\r\n", decimal, value, unit_str);
}

/**
  * @brief  串口空闲中断回调函数
  * @note   在USART1_IRQHandler中调用
  * @retval None
  */
void UART_BT_IdleCallback(void)
{
    // 停止DMA传输
    HAL_UART_DMAStop(&huart1);
    
    // 计算接收到的数据长度
    rx_data_length = UART_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx);
    
    // TODO: 在这里处理接收到的数据
    // 可以解析蓝牙命令或转发数据
    // 示例：
    // if (rx_data_length > 0) {
    //     // 处理 uart_rx_buffer 中的数据
    // }
    
    // 清空缓冲区准备下次接收
    memset(uart_rx_buffer, 0, UART_RX_BUFFER_SIZE);
    rx_data_length = 0;
    
    // 重新启动DMA接收
    HAL_UART_Receive_DMA(&huart1, uart_rx_buffer, UART_RX_BUFFER_SIZE);
}

/**
  * @brief  获取接收数据长度
  * @retval 接收到的数据长度
  */
uint16_t UART_BT_GetRxDataLength(void)
{
    return rx_data_length;
}

/**
  * @brief  获取接收缓冲区指针
  * @retval 接收缓冲区指针
  */
uint8_t* UART_BT_GetRxBuffer(void)
{
    return uart_rx_buffer;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  串口发送完成回调
  * @note   在HAL_UART_TxCpltCallback中调用
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        tx_busy = 0;
    }
}
