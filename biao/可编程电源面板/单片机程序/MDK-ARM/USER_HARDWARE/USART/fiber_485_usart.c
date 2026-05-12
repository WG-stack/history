#include "fiber_485_usart.h"
#include "usart.h"
#include <string.h>

/* 光纤/485串口接收缓冲区定义 */
USART_REC_U3 RS485_1_USART = {0};

/**
 * @brief 光纤/485通讯串口初始化
 * @note USART3用于光纤通讯或485通讯(二选一)，使能DMA接收和空闲中断
 */
void FIBER_485_USART_Init(void)
{
    FIBER_485_USART_ClearRxBuffer();
    
    // USART3的DMA接收和空闲中断已在CubeMX中配置
    // 这里可以添加其他初始化代码
}

/**
 * @brief 发送一个字节
 * @param data 要发送的字节
 * @note 使用DMA发送
 */
void FIBER_485_USART_SendByte(uint8_t data)
{
    HAL_UART_Transmit_DMA(&huart3, &data, 1);
}

/**
 * @brief 发送数据
 * @param data 数据指针
 * @param len 数据长度
 * @note 使用DMA发送
 */
void FIBER_485_USART_SendData(const uint8_t *data, uint16_t len)
{
    if(data != NULL && len > 0)
    {
        HAL_UART_Transmit_DMA(&huart3, (uint8_t*)data, len);
    }
}

/**
 * @brief 发送字符串
 * @param str 字符串指针
 * @note 使用DMA发送
 */
void FIBER_485_USART_SendString(const char *str)
{
    if(str != NULL)
    {
        uint16_t len = 0;
        while(str[len] != '\0')
        {
            len++;
        }
        HAL_UART_Transmit_DMA(&huart3, (uint8_t*)str, len);
    }
}

/**
 * @brief 数据解析处理
 * @note 协议待定，当前为占位实现
 *       根据具体通讯协议实现数据解析
 */
void FIBER_485_USART_Parse(void)
{
    if(RS485_1_USART.USART_DMA_RX_STA == 1)
    {
        // TODO: 根据具体通讯协议解析接收到的数据
        // 示例：
        // uint8_t *data = RS485_1_USART.USART_DMA_REbuffer;
        // uint16_t len = RS485_1_USART.USART_RE_length;
        
        // 解析完成后清除标志
        RS485_1_USART.USART_DMA_RX_STA = 0;
    }
}

/**
 * @brief 清空接收缓冲区
 */
void FIBER_485_USART_ClearRxBuffer(void)
{
    RS485_1_USART.USART_DMA_RX_STA = 0;
    RS485_1_USART.USART_RE_length = 0;
    memset(RS485_1_USART.USART_DMA_REbuffer, 0, sizeof(RS485_1_USART.USART_DMA_REbuffer));
}

/**
 * @brief 获取接收数据长度
 * @return 接收到的数据长度
 */
uint16_t FIBER_485_USART_GetRxLength(void)
{
    return RS485_1_USART.USART_RE_length;
}

/**
 * @brief 获取接收缓冲区指针
 * @return 接收缓冲区指针
 */
uint8_t* FIBER_485_USART_GetRxBuffer(void)
{
    return RS485_1_USART.USART_DMA_REbuffer;
}

/**
 * @brief 检查DMA发送是否完成
 * @return 0-发送中, 1-发送完成
 */
uint8_t FIBER_485_USART_IsTxComplete(void)
{
    return (huart3.gState == HAL_UART_STATE_READY) ? 1 : 0;
}
