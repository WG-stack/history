#include "rx8025t.h"

/* I2C引脚操作宏定义 (PC2-SCL, PC3-SDA) */
#define RX8025T_SCL_H()  PCout(2) = 1       // SCL输出高电平
#define RX8025T_SCL_L()  PCout(2) = 0       // SCL输出低电平
#define RX8025T_SDA_H()  PCout(3) = 1       // SDA输出高电平
#define RX8025T_SDA_L()  PCout(3) = 0       // SDA输出低电平
#define RX8025T_SDA_READ() PCin(3)          // 读取SDA电平

/**
 * @brief I2C延时函数
 */
static void RX8025T_Delay(void)
{
    delay_us(5);
}

/**
 * @brief 设置SDA为输出模式
 */
static void RX8025T_SDA_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = RX8025T_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RX8025T_SDA_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief 设置SDA为输入模式
 */
static void RX8025T_SDA_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = RX8025T_SDA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(RX8025T_SDA_GPIO_Port, &GPIO_InitStruct);
}

/**
 * @brief I2C起始信号
 */
static void RX8025T_Start(void)
{
    RX8025T_SDA_OUT();
    RX8025T_SDA_H();
    RX8025T_SCL_H();
    RX8025T_Delay();
    RX8025T_SDA_L();
    RX8025T_Delay();
    RX8025T_SCL_L();
}

/**
 * @brief I2C停止信号
 */
static void RX8025T_Stop(void)
{
    RX8025T_SDA_OUT();
    RX8025T_SCL_L();
    RX8025T_SDA_L();
    RX8025T_Delay();
    RX8025T_SCL_H();
    RX8025T_Delay();
    RX8025T_SDA_H();
    RX8025T_Delay();
}

/**
 * @brief 等待应答信号
 * @return 0-收到应答, 1-未收到应答
 */
static uint8_t RX8025T_WaitAck(void)
{
    uint8_t timeout = 0;
    RX8025T_SDA_IN();
    RX8025T_SDA_H();
    RX8025T_Delay();
    RX8025T_SCL_H();
    RX8025T_Delay();
    
    while(RX8025T_SDA_READ())
    {
        timeout++;
        if(timeout > 250)
        {
            RX8025T_Stop();
            return 1;
        }
    }
    RX8025T_SCL_L();
    return 0;
}

/**
 * @brief 发送应答信号
 */
static void RX8025T_Ack(void)
{
    RX8025T_SCL_L();
    RX8025T_SDA_OUT();
    RX8025T_SDA_L();
    RX8025T_Delay();
    RX8025T_SCL_H();
    RX8025T_Delay();
    RX8025T_SCL_L();
}

/**
 * @brief 发送非应答信号
 */
static void RX8025T_NAck(void)
{
    RX8025T_SCL_L();
    RX8025T_SDA_OUT();
    RX8025T_SDA_H();
    RX8025T_Delay();
    RX8025T_SCL_H();
    RX8025T_Delay();
    RX8025T_SCL_L();
}

/**
 * @brief I2C发送一个字节
 * @param byte 要发送的字节
 */
static void RX8025T_SendByte(uint8_t byte)
{
    uint8_t i;
    RX8025T_SDA_OUT();
    RX8025T_SCL_L();
    
    for(i = 0; i < 8; i++)
    {
        if(byte & 0x80)
            RX8025T_SDA_H();
        else
            RX8025T_SDA_L();
        
        byte <<= 1;
        RX8025T_Delay();
        RX8025T_SCL_H();
        RX8025T_Delay();
        RX8025T_SCL_L();
        RX8025T_Delay();
    }
}

/**
 * @brief I2C读取一个字节
 * @param ack 1-发送应答, 0-发送非应答
 * @return 读取到的字节
 */
static uint8_t RX8025T_ReadByte(uint8_t ack)
{
    uint8_t i, byte = 0;
    RX8025T_SDA_IN();
    
    for(i = 0; i < 8; i++)
    {
        RX8025T_SCL_L();
        RX8025T_Delay();
        RX8025T_SCL_H();
        byte <<= 1;
        if(RX8025T_SDA_READ())
            byte |= 0x01;
        RX8025T_Delay();
    }
    
    if(ack)
        RX8025T_Ack();
    else
        RX8025T_NAck();
    
    return byte;
}

/**
 * @brief RX8025T初始化
 */
void RX8025T_Init(void)
{
    RX8025T_SCL_H();
    RX8025T_SDA_H();
}

/**
 * @brief 写RX8025T寄存器
 * @param reg 寄存器地址
 * @param data 要写入的数据
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_WriteReg(uint8_t reg, uint8_t data)
{
    RX8025T_Start();
    RX8025T_SendByte(RX8025T_ADDR_WRITE);
    if(RX8025T_WaitAck())
        return 1;
    
    RX8025T_SendByte(reg);
    if(RX8025T_WaitAck())
        return 1;
    
    RX8025T_SendByte(data);
    if(RX8025T_WaitAck())
        return 1;
    
    RX8025T_Stop();
    return 0;
}

/**
 * @brief 读RX8025T寄存器
 * @param reg 寄存器地址
 * @param data 读取数据的指针
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_ReadReg(uint8_t reg, uint8_t *data)
{
    RX8025T_Start();
    RX8025T_SendByte(RX8025T_ADDR_WRITE);
    if(RX8025T_WaitAck())
        return 1;
    
    RX8025T_SendByte(reg);
    if(RX8025T_WaitAck())
        return 1;
    
    RX8025T_Start();
    RX8025T_SendByte(RX8025T_ADDR_READ);
    if(RX8025T_WaitAck())
        return 1;
    
    *data = RX8025T_ReadByte(0);
    RX8025T_Stop();
    return 0;
}

/**
 * @brief BCD码转十进制
 * @param bcd BCD码
 * @return 十进制数
 */
uint8_t BCD_To_DEC(uint8_t bcd)
{
    return ((bcd >> 4) * 10 + (bcd & 0x0F));
}

/**
 * @brief 十进制转BCD码
 * @param dec 十进制数
 * @return BCD码
 */
uint8_t DEC_To_BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

/**
 * @brief 设置RTC时间
 * @param time 时间结构体指针
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_SetTime(RX8025T_Time *time)
{
    if(RX8025T_WriteReg(RX8025T_REG_SEC, DEC_To_BCD(time->second)))
        return 1;
    if(RX8025T_WriteReg(RX8025T_REG_MIN, DEC_To_BCD(time->minute)))
        return 1;
    if(RX8025T_WriteReg(RX8025T_REG_HOUR, DEC_To_BCD(time->hour)))
        return 1;
    if(RX8025T_WriteReg(RX8025T_REG_WEEK, DEC_To_BCD(time->week)))
        return 1;
    if(RX8025T_WriteReg(RX8025T_REG_DAY, DEC_To_BCD(time->day)))
        return 1;
    if(RX8025T_WriteReg(RX8025T_REG_MONTH, DEC_To_BCD(time->month)))
        return 1;
    if(RX8025T_WriteReg(RX8025T_REG_YEAR, DEC_To_BCD(time->year)))
        return 1;
    
    return 0;
}

/**
 * @brief 获取RTC时间
 * @param time 时间结构体指针
 * @return 0-成功, 1-失败
 */
uint8_t RX8025T_GetTime(RX8025T_Time *time)
{
    uint8_t temp;
    
    if(RX8025T_ReadReg(RX8025T_REG_SEC, &temp))
        return 1;
    time->second = BCD_To_DEC(temp & 0x7F);
    
    if(RX8025T_ReadReg(RX8025T_REG_MIN, &temp))
        return 1;
    time->minute = BCD_To_DEC(temp & 0x7F);
    
    if(RX8025T_ReadReg(RX8025T_REG_HOUR, &temp))
        return 1;
    time->hour = BCD_To_DEC(temp & 0x3F);
    
    if(RX8025T_ReadReg(RX8025T_REG_WEEK, &temp))
        return 1;
    time->week = BCD_To_DEC(temp & 0x07);
    
    if(RX8025T_ReadReg(RX8025T_REG_DAY, &temp))
        return 1;
    time->day = BCD_To_DEC(temp & 0x3F);
    
    if(RX8025T_ReadReg(RX8025T_REG_MONTH, &temp))
        return 1;
    time->month = BCD_To_DEC(temp & 0x1F);
    
    if(RX8025T_ReadReg(RX8025T_REG_YEAR, &temp))
        return 1;
    time->year = BCD_To_DEC(temp);
    
    return 0;
}
