#ifndef _HMI_USART_H_
#define _HMI_USART_H_

#include "main.h"

/**
 * @brief HMI串口初始化
 * @note USART1用于HMI屏幕通讯，使能DMA接收
 */
void HMI_USART_Init(void);

/**
 * @brief 发送一个字节到HMI
 * @param data 要发送的字节
 */
void HMI_USART_SendByte(uint8_t data);

/**
 * @brief 发送数据到HMI
 * @param data 数据指针
 * @param len 数据长度
 */
void HMI_USART_SendData(const uint8_t *data, uint16_t len);

/**
 * @brief 发送字符串到HMI
 * @param str 字符串指针
 */
void HMI_USART_SendString(const char *str);

/**
 * @brief HMI数据解析处理
 * @note 协议待定，当前为占位实现
 */
void HMI_USART_Parse(void);

/**
 * @brief 清空HMI接收缓冲区
 */
void HMI_USART_ClearRxBuffer(void);

/**
 * @brief 获取HMI接收数据长度
 * @return 接收到的数据长度
 */
uint16_t HMI_USART_GetRxLength(void);

/**
 * @brief 获取HMI接收缓冲区指针
 * @return 接收缓冲区指针
 */
uint8_t* HMI_USART_GetRxBuffer(void);

/**
 * @brief 检查DMA发送是否完成
 * @return 0-发送中, 1-发送完成
 */
uint8_t HMI_USART_IsTxComplete(void);

#endif
