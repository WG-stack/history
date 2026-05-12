#ifndef _FIBER_485_USART_H_
#define _FIBER_485_USART_H_

#include "main.h"

/**
 * @brief 光纤/485通讯串口初始化
 * @note USART3用于光纤通讯或485通讯(二选一)，使能DMA接收
 */
void FIBER_485_USART_Init(void);

/**
 * @brief 发送一个字节
 * @param data 要发送的字节
 */
void FIBER_485_USART_SendByte(uint8_t data);

/**
 * @brief 发送数据
 * @param data 数据指针
 * @param len 数据长度
 */
void FIBER_485_USART_SendData(const uint8_t *data, uint16_t len);

/**
 * @brief 发送字符串
 * @param str 字符串指针
 */
void FIBER_485_USART_SendString(const char *str);

/**
 * @brief 数据解析处理
 * @note 协议待定，当前为占位实现
 */
void FIBER_485_USART_Parse(void);

/**
 * @brief 清空接收缓冲区
 */
void FIBER_485_USART_ClearRxBuffer(void);

/**
 * @brief 获取接收数据长度
 * @return 接收到的数据长度
 */
uint16_t FIBER_485_USART_GetRxLength(void);

/**
 * @brief 获取接收缓冲区指针
 * @return 接收缓冲区指针
 */
uint8_t* FIBER_485_USART_GetRxBuffer(void);

/**
 * @brief 检查DMA发送是否完成
 * @return 0-发送中, 1-发送完成
 */
uint8_t FIBER_485_USART_IsTxComplete(void);

#endif
