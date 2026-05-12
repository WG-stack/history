#include "at24c64.h"

/* I2C引脚操作宏定义 (PC0-SCL, PC1-SDA) */
#define AT24_SCL_H()  PCout(0) = 1      // SCL输出高电平
#define AT24_SCL_L()  PCout(0) = 0      // SCL输出低电平
#define AT24_SDA_H()  PCout(1) = 1      // SDA输出高电平
#define AT24_SDA_L()  PCout(1) = 0      // SDA输出低电平
#define AT24_SDA_READ() PCin(1)         // 读取SDA电平

/**
 * @brief I2C延时函数
 */
static void AT24_Delay(void)
{
    delay_us(5);
}

/**
 * @brief 设置SDA为输出模式
 */
static void AT24_SDA_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = AT24_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(AT24_SDA_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief 设置SDA为输入模式
 */
static void AT24_SDA_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = AT24_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(AT24_SDA_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief I2C起始信号
 */
static void AT24_Start(void)
{
    AT24_SDA_OUT();
    AT24_SDA_H();
    AT24_SCL_H();
    AT24_Delay();
    AT24_SDA_L();
    AT24_Delay();
    AT24_SCL_L();
}

/**
 * @brief I2C停止信号
 */
static void AT24_Stop(void)
{
    AT24_SDA_OUT();
    AT24_SCL_L();
    AT24_SDA_L();
    AT24_Delay();
    AT24_SCL_H();
    AT24_Delay();
    AT24_SDA_H();
    AT24_Delay();
}

/**
 * @brief 等待应答信号
 * @return 0-收到应答, 1-未收到应答
 */
static uint8_t AT24_WaitAck(void)
{
    uint8_t timeout = 0;
    AT24_SDA_IN();
    AT24_SDA_H();
    AT24_Delay();
    AT24_SCL_H();
    AT24_Delay();
    
    while(AT24_SDA_READ())
    {
        timeout++;
        if(timeout > 250)
        {
            AT24_Stop();
            return 1;
        }
    }
    AT24_SCL_L();
    return 0;
}

/**
 * @brief 发送应答信号
 */
static void AT24_Ack(void)
{
    AT24_SCL_L();
    AT24_SDA_OUT();
    AT24_SDA_L();
    AT24_Delay();
    AT24_SCL_H();
    AT24_Delay();
    AT24_SCL_L();
}

/**
 * @brief 发送非应答信号
 */
static void AT24_NAck(void)
{
    AT24_SCL_L();
    AT24_SDA_OUT();
    AT24_SDA_H();
    AT24_Delay();
    AT24_SCL_H();
    AT24_Delay();
    AT24_SCL_L();
}

/**
 * @brief I2C发送一个字节
 * @param byte 要发送的字节
 */
static void AT24_SendByte(uint8_t byte)
{
    uint8_t i;
    AT24_SDA_OUT();
    AT24_SCL_L();
    
    for(i = 0; i < 8; i++)
    {
        if(byte & 0x80)
            AT24_SDA_H();
        else
            AT24_SDA_L();
        
        byte <<= 1;
        AT24_Delay();
        AT24_SCL_H();
        AT24_Delay();
        AT24_SCL_L();
        AT24_Delay();
    }
}

/**
 * @brief I2C读取一个字节
 * @param ack 1-发送应答, 0-发送非应答
 * @return 读取到的字节
 */
static uint8_t AT24_ReadByte(uint8_t ack)
{
    uint8_t i, byte = 0;
    AT24_SDA_IN();
    
    for(i = 0; i < 8; i++)
    {
        AT24_SCL_L();
        AT24_Delay();
        AT24_SCL_H();
        byte <<= 1;
        if(AT24_SDA_READ())
            byte |= 0x01;
        AT24_Delay();
    }
    
    if(ack)
        AT24_Ack();
    else
        AT24_NAck();
    
    return byte;
}

/**
 * @brief AT24C64初始化
 */
void AT24C64_Init(void)
{
    AT24_SCL_H();
    AT24_SDA_H();
}

/**
 * @brief 检测AT24C64是否正常
 * @return 0-正常, 1-异常
 */
uint8_t AT24C64_Check(void)
{
    uint8_t temp;
    uint8_t test_data = 0x55;
    
    if(AT24C64_ReadByte(0, &temp))
        return 1;
    
    if(temp == test_data)
        return 0;
    
    if(AT24C64_WriteByte(0, test_data))
        return 1;
    
    HAL_Delay(10);
    
    if(AT24C64_ReadByte(0, &temp))
        return 1;
    
    if(temp == test_data)
        return 0;
    else
        return 1;
}

/**
 * @brief 写一个字节到AT24C64
 * @param addr 写入地址 (0-8191)
 * @param data 要写入的数据
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_WriteByte(uint16_t addr, uint8_t data)
{
    if(addr >= AT24C64_MAX_ADDR)
        return 1;
    
    AT24_Start();
    AT24_SendByte(AT24C64_ADDR_WRITE);
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr >> 8));
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr & 0xFF));
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte(data);
    if(AT24_WaitAck())
        return 1;
    
    AT24_Stop();
    HAL_Delay(5);
    return 0;
}

/**
 * @brief 从AT24C64读一个字节
 * @param addr 读取地址 (0-8191)
 * @param data 读取数据的指针
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_ReadByte(uint16_t addr, uint8_t *data)
{
    if(addr >= AT24C64_MAX_ADDR)
        return 1;
    
    AT24_Start();
    AT24_SendByte(AT24C64_ADDR_WRITE);
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr >> 8));
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr & 0xFF));
    if(AT24_WaitAck())
        return 1;
    
    AT24_Start();
    AT24_SendByte(AT24C64_ADDR_READ);
    if(AT24_WaitAck())
        return 1;
    
    *data = AT24_ReadByte(0);
    AT24_Stop();
    return 0;
}

/**
 * @brief 页写入(最多32字节)
 * @param addr 起始地址
 * @param pData 数据指针
 * @param len 数据长度
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_WritePage(uint16_t addr, uint8_t *pData, uint16_t len)
{
    uint16_t i;
    
    if(addr >= AT24C64_MAX_ADDR || len == 0)
        return 1;
    
    if((addr % AT24C64_PAGE_SIZE) + len > AT24C64_PAGE_SIZE)
        return 1;
    
    AT24_Start();
    AT24_SendByte(AT24C64_ADDR_WRITE);
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr >> 8));
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr & 0xFF));
    if(AT24_WaitAck())
        return 1;
    
    for(i = 0; i < len; i++)
    {
        AT24_SendByte(pData[i]);
        if(AT24_WaitAck())
            return 1;
    }
    
    AT24_Stop();
    HAL_Delay(5);
    return 0;
}

/**
 * @brief 连续读取多个字节
 * @param addr 起始地址
 * @param pData 数据缓冲区指针
 * @param len 读取长度
 * @return 0-成功, 1-失败
 */
uint8_t AT24C64_ReadBytes(uint16_t addr, uint8_t *pData, uint16_t len)
{
    uint16_t i;
    
    if(addr >= AT24C64_MAX_ADDR || len == 0)
        return 1;
    
    AT24_Start();
    AT24_SendByte(AT24C64_ADDR_WRITE);
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr >> 8));
    if(AT24_WaitAck())
        return 1;
    
    AT24_SendByte((uint8_t)(addr & 0xFF));
    if(AT24_WaitAck())
        return 1;
    
    AT24_Start();
    AT24_SendByte(AT24C64_ADDR_READ);
    if(AT24_WaitAck())
        return 1;
    
    for(i = 0; i < len; i++)
    {
        if(i == len - 1)
            pData[i] = AT24_ReadByte(0);
        else
            pData[i] = AT24_ReadByte(1);
    }
    
    AT24_Stop();
    return 0;
}
