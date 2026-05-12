#include "hmi_usart.h"
#include "usart.h"
#include <string.h>

/* HMI串口接收缓冲区定义 */
USART_REC_U1 HMI_USART = {0};

/**
 * @brief HMI串口初始化
 * @note USART1用于HMI屏幕通讯，使能DMA接收和空闲中断
 */
void HMI_USART_Init(void)
{
    HMI_USART_ClearRxBuffer();
    
    // USART1的DMA接收和空闲中断已在CubeMX中配置
    // 这里可以添加其他初始化代码
}

/**
 * @brief 发送一个字节到HMI
 * @param data 要发送的字节
 * @note 使用DMA发送
 */
void HMI_USART_SendByte(uint8_t data)
{
    HAL_UART_Transmit_DMA(&huart1, &data, 1);
}

/**
 * @brief 发送数据到HMI
 * @param data 数据指针
 * @param len 数据长度
 * @note 使用DMA发送
 */
void HMI_USART_SendData(const uint8_t *data, uint16_t len)
{
    if(data != NULL && len > 0)
    {
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)data, len);
    }
}

/**
 * @brief 发送字符串到HMI
 * @param str 字符串指针
 * @note 使用DMA发送
 */
void HMI_USART_SendString(const char *str)
{
    if(str != NULL)
    {
        uint16_t len = 0;
        while(str[len] != '\0')
        {
            len++;
        }
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)str, len);
    }
}

/**
 * @brief HMI数据解析处理
 * @note 协议待定，当前为占位实现
 *       根据具体HMI协议实现数据解析
 */
void HMI_USART_Parse(void)
{
    if(HMI_USART.USART_DMA_RX_STA == 1)
    {
        // TODO: 根据HMI协议解析接收到的数据
        // 示例：
        // uint8_t *data = HMI_USART.USART_DMA_REbuffer;
        // uint16_t len = HMI_USART.USART_RE_length;
        
        // 解析完成后清除标志
        HMI_USART.USART_DMA_RX_STA = 0;
    }
}

/**
 * @brief 清空HMI接收缓冲区
 */
void HMI_USART_ClearRxBuffer(void)
{
    HMI_USART.USART_DMA_RX_STA = 0;
    HMI_USART.USART_RE_length = 0;
    memset(HMI_USART.USART_DMA_REbuffer, 0, sizeof(HMI_USART.USART_DMA_REbuffer));
}

/**
 * @brief 获取HMI接收数据长度
 * @return 接收到的数据长度
 */
uint16_t HMI_USART_GetRxLength(void)
{
    return HMI_USART.USART_RE_length;
}

/**
 * @brief 获取HMI接收缓冲区指针
 * @return 接收缓冲区指针
 */
uint8_t* HMI_USART_GetRxBuffer(void)
{
    return HMI_USART.USART_DMA_REbuffer;
}

/**
 * @brief 检查DMA发送是否完成
 * @return 0-发送中, 1-发送完成
 */
uint8_t HMI_USART_IsTxComplete(void)
{
    return (huart1.gState == HAL_UART_STATE_READY) ? 1 : 0;
}
