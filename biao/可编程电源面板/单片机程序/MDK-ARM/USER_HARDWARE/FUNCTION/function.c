#include "function.h"
#include <stdio.h>

/* 测试模式标志 */
static uint8_t test_mode = 1;  // 1-测试模式, 0-正常模式
static uint32_t test_counter = 0;
static uint32_t test_cycle = 0;

/**
 * @brief 发送测试信息到调试串口
 * @param msg 要发送的消息字符串
 * @note 同时发送到HMI和RS485固定输出串口
 */
static void Test_SendMsg(const char *msg)
{
    HMI_USART_SendString(msg);
    RS485_USART_SendString(msg);
}

/**
 * @brief 底层基本测试函数
 * @note 用于初期底层调通，测试所有外设功能
 */
static void Hardware_Test(void)
{
    static uint32_t last_tick = 0;
    uint32_t current_tick = HAL_GetTick();
    
    // 每1秒执行一次测试
    if(current_tick - last_tick < 1000)
        return;
    
    last_tick = current_tick;
    test_cycle++;
    
    char test_msg[100];
    sprintf(test_msg, "\r\n=== 测试周期 %d ===\r\n", (int)test_cycle);
    Test_SendMsg(test_msg);
    
    // ========== 1. RTC测试 ==========
    RX8025T_Time rtc_time;
    
    if(RX8025T_GetTime(&rtc_time) == 0)
    {
        sprintf(test_msg, "[RTC时间] %02d:%02d:%02d\r\n", 
                rtc_time.hour, rtc_time.minute, rtc_time.second);
        Test_SendMsg(test_msg);
        sprintf(test_msg, "[RTC日期] 20%02d-%02d-%02d 星期%d\r\n", 
                rtc_time.year, rtc_time.month, rtc_time.day, rtc_time.week);
        Test_SendMsg(test_msg);
    }
    else
    {
        Test_SendMsg("[RTC] 读取失败!\r\n");
    }
    
    // ========== 2. EEPROM测试 ==========
    uint8_t write_data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t read_data[8] = {0};
    
    if(AT24C64_WritePage(0x0000, write_data, 8) == 0)
    {
        HAL_Delay(10);  // 等待写入完成
        if(AT24C64_ReadBytes(0x0000, read_data, 8) == 0)
        {
            uint8_t match = 1;
            for(int i = 0; i < 8; i++)
            {
                if(write_data[i] != read_data[i])
                {
                    match = 0;
                    break;
                }
            }
            if(match)
                Test_SendMsg("[EEPROM] 读写测试通过\r\n");
            else
                Test_SendMsg("[EEPROM] 数据不匹配!\r\n");
        }
        else
        {
            Test_SendMsg("[EEPROM] 读取失败!\r\n");
        }
    }
    else
    {
        Test_SendMsg("[EEPROM] 写入失败!\r\n");
    }
    
    // ========== 3. BCD输出测试 ==========
    uint8_t bcd_value = (test_cycle % 128);  // 0-127循环
    BCD_SetOutput(bcd_value);
    sprintf(test_msg, "[BCD输出] 当前值: %d\r\n", bcd_value);
    Test_SendMsg(test_msg);
    
    // ========== 4. ADC采样测试 ==========
    float adc_current = ADC_GetCurrent();
    float adc_voltage = ADC_GetVoltage();
    sprintf(test_msg, "[ADC] 电流: %.3fA, 电压: %.3fV\r\n", 
            adc_current, adc_voltage);
    Test_SendMsg(test_msg);
    
    // ========== 5. DAC输出测试 ==========
    // 输出正弦波测试值
    static float dac_phase = 0;
    dac_phase += 0.1f;
    if(dac_phase > 6.28f) dac_phase = 0;
    
    float dac_voltage = 3.0f + 1.0f * sinf(dac_phase);  // 2.0V-4.0V
    float dac_current = 12.0f + 4.0f * cosf(dac_phase);  // 8mA-16mA
    
    DAC_SetVoltage(dac_voltage);
    DAC_SetCurrent(dac_current);
    sprintf(test_msg, "[DAC] 电压: %.3fV, 电流: %.3fmA\r\n", 
            dac_voltage, dac_current);
    Test_SendMsg(test_msg);
    
    // ========== 6. 指令输入测试 ==========
    CMD_Input_Scan();
    uint8_t cmd_state = CMD_Input_GetAll();
    sprintf(test_msg, "[指令输入] 状态: 0x%02X (CMD1:%d CMD2:%d CMD3:%d CMD4:%d)\r\n",
            cmd_state,
            CMD_Input_Read(1), CMD_Input_Read(2), 
            CMD_Input_Read(3), CMD_Input_Read(4));
    Test_SendMsg(test_msg);
    
    // 检查状态变化
    for(uint8_t i = 1; i <= 4; i++)
    {
        if(CMD_Input_IsChanged(i))
        {
            sprintf(test_msg, "[指令输入] CMD%d 状态变化: %d\r\n", 
                    i, CMD_Input_Read(i));
            Test_SendMsg(test_msg);
            CMD_Input_ClearChanged(i);
        }
    }
    
    // ========== 7. 串口通讯测试 ==========
    // 发送测试数据到各串口
    sprintf(test_msg, "HMI Test %d\r\n", (int)test_cycle);
    HMI_USART_SendString(test_msg);
    
    sprintf(test_msg, "485_1 Test %d\r\n", (int)test_cycle);
    FIBER_485_USART_SendString(test_msg);
    
    sprintf(test_msg, "485_2 Test %d\r\n", (int)test_cycle);
    RS485_USART_SendString(test_msg);
    
    // 检查串口接收
    if(HMI_USART.USART_DMA_RX_STA == 1)
    {
        sprintf(test_msg, "[HMI接收] 长度:%d 数据:", HMI_USART.USART_RE_length);
        HMI_USART_SendString(test_msg);
        HMI_USART_SendData(HMI_USART.USART_DMA_REbuffer, HMI_USART.USART_RE_length);
        HMI_USART_SendString("\r\n");
        HMI_USART.USART_DMA_RX_STA = 0;
    }
    
    if(RS485_1_USART.USART_DMA_RX_STA == 1)
    {
        sprintf(test_msg, "[485_1接收] 长度:%d\r\n", RS485_1_USART.USART_RE_length);
        Test_SendMsg(test_msg);
        RS485_1_USART.USART_DMA_RX_STA = 0;
    }
    
    if(RS485_2_USART.USART_DMA_RX_STA == 1)
    {
        sprintf(test_msg, "[485_2接收] 长度:%d\r\n", RS485_2_USART.USART_RE_length);
        Test_SendMsg(test_msg);
        RS485_2_USART.USART_DMA_RX_STA = 0;
    }
    
    Test_SendMsg("=== 测试完成 ===\r\n\r\n");
}

/**
 * @brief 主功能函数，在main循环中调用
 * @note 包含测试模式和正常工作模式
 */
void MainFunction(void)
{
    test_counter++;
    
    if(test_mode)
    {
        // 测试模式：执行底层硬件测试
        Hardware_Test();
        
        // 可通过串口命令切换到正常模式
        // 接收到 "EXIT_TEST" 命令时退出测试模式
        if(HMI_USART.USART_DMA_RX_STA == 1)
        {
            if(HMI_USART.USART_RE_length >= 9)
            {
                if(strncmp((char*)HMI_USART.USART_DMA_REbuffer, "EXIT_TEST", 9) == 0)
                {
                    test_mode = 0;
                    HMI_USART_SendString("\r\n>>> 退出测试模式，进入正常工作模式 <<<\r\n\r\n");
                }
            }
            HMI_USART.USART_DMA_RX_STA = 0;
        }
    }
    else
    {
        // 正常工作模式：在这里添加实际业务逻辑
        
        // 可通过串口命令重新进入测试模式
        if(HMI_USART.USART_DMA_RX_STA == 1)
        {
            if(HMI_USART.USART_RE_length >= 10)
            {
                if(strncmp((char*)HMI_USART.USART_DMA_REbuffer, "ENTER_TEST", 10) == 0)
                {
                    test_mode = 1;
                    test_cycle = 0;
                    HMI_USART_SendString("\r\n>>> 进入测试模式 <<<\r\n\r\n");
                }
            }
            HMI_USART.USART_DMA_RX_STA = 0;
        }
        
        // TODO: 添加正常工作模式的业务逻辑
    }
}
