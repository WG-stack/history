/**
 * @file    analog_pointer_test.c
 * @brief   模拟指针显示测试文件
 * @author  Roo
 * @date    2026-02-07
 * 
 * 用于测试模拟指针显示功能的修复效果
 */

#include "data_processing.h"
#include "global_variables.h"
#include "measurement_state_machine.h"

/**
 * @brief 测试模拟指针计算函数
 * @param test_value 测试值
 * @param resolution 测试分辨率
 * @retval 指针计算结果
 */
int8_t TestAnalogPointerCalculation(int32_t test_value, Resolution_TypeDef resolution)
{
    int8_t result = DataProcessing_CalculateAnalogPointerValue(test_value, resolution);
    
    // 打印测试结果（如果启用了调试输出）
    #ifdef DEBUG_ANALOG_POINTER
    char debug_str[64];
    sprintf(debug_str, "Test: Value=%d, Res=%d, Pointer=%d", test_value, resolution, result);
    // 这里可以添加串口输出或其他调试输出
    #endif
    
    return result;
}

/**
 * @brief 测试TIR模式下的指针显示逻辑
 * @param current_deviation 当前偏差
 * @param max_positive_dev 最大正偏差
 * @param max_negative_dev 最大负偏差
 * @retval 测试结果
 */
void TestTirPointerLogic(int32_t current_deviation, int32_t max_positive_dev, int32_t max_negative_dev)
{
    // 模拟TIR模式下的指针计算逻辑
    int ps_level = 0;
    int analog_value = 0;
    
    if(current_deviation > 0) {
        // 计算相对于最大正跳动的比例，然后映射到1-15的图标
        // 修复除零错误：当max_positive_dev为0时，使用当前偏差的绝对值作为参考
        float reference_value = (max_positive_dev == 0) ? (float)current_deviation : (float)max_positive_dev;
        float ratio = (float)current_deviation / reference_value;
        
        // 限制比例在合理范围内
        if(ratio > 1.0f) ratio = 1.0f;
        if(ratio < 0.0f) ratio = 0.0f;
        
        ps_level = (int)(ratio * 15);
        if(ps_level > 15) ps_level = 15;
        if(ps_level < 1) ps_level = 1;
        
        analog_value = ps_level;
        
        #ifdef DEBUG_ANALOG_POINTER
        char debug_str[64];
        sprintf(debug_str, "TIR+: Dev=%d, Max=%d, Ratio=%.2f, Level=%d", 
                current_deviation, max_positive_dev, ratio, ps_level);
        // 添加调试输出
        #endif
    } else if(current_deviation < 0) {
        // 计算相对于最大负跳动的比例，然后映射到1-15的图标
        // 修复除零错误：当max_negative_dev为0时，使用当前偏差的绝对值作为参考
        int32_t abs_max_negative_dev = (max_negative_dev == 0) ? (-current_deviation) : (-max_negative_dev);
        float ratio = (float)(-current_deviation) / (float)abs_max_negative_dev;
        
        // 限制比例在合理范围内
        if(ratio > 1.0f) ratio = 1.0f;
        if(ratio < 0.0f) ratio = 0.0f;
        
        ps_level = (int)(ratio * 15);
        if(ps_level > 15) ps_level = 15;
        if(ps_level < 1) ps_level = 1;
        
        analog_value = -ps_level;
        
        #ifdef DEBUG_ANALOG_POINTER
        char debug_str[64];
        sprintf(debug_str, "TIR-: Dev=%d, Max=%d, Ratio=%.2f, Level=%d", 
                current_deviation, max_negative_dev, ratio, ps_level);
        // 添加调试输出
        #endif
    } else {
        // 当前偏差为0，指针值为0
        analog_value = 0;
        
        #ifdef DEBUG_ANALOG_POINTER
        char debug_str[32];
        sprintf(debug_str, "TIR0: Dev=0, Pointer=0");
        // 添加调试输出
        #endif
    }
    
    // 这里可以添加实际的LCD显示测试
    // LCDDisplay_ShowChar_New(...);
}

/**
 * @brief 运行完整的模拟指针测试
 * @retval 测试结果状态
 */
uint8_t RunAnalogPointerTests(void)
{
    uint8_t test_results = 0;
    
    #ifdef DEBUG_ANALOG_POINTER
    // 启用调试输出
    #endif
    
    // 测试1：基本指针计算
    test_results |= (TestAnalogPointerCalculation(1000, RESOLUTION_0_001MM) == 1) << 0;
    test_results |= (TestAnalogPointerCalculation(-1000, RESOLUTION_0_001MM) == -1) << 1;
    test_results |= (TestAnalogPointerCalculation(0, RESOLUTION_0_001MM) == 0) << 2;
    
    // 测试边界值
    test_results |= (TestAnalogPointerCalculation(10000, RESOLUTION_0_001MM) <= 15) << 3;
    test_results |= (TestAnalogPointerCalculation(-10000, RESOLUTION_0_001MM) >= -15) << 4;
    
    // 测试TIR模式逻辑
    TestTirPointerLogic(500, 1000, -800);  // 正常情况
    TestTirPointerLogic(0, 0, 0);          // 边界情况 - 所有值为0
    TestTirPointerLogic(2000, 1000, -500); // 超出范围的情况
    
    return test_results;
}

/**
 * @brief 打印测试结果
 * @param results 测试结果
 */
void PrintTestResults(uint8_t results)
{
    #ifdef DEBUG_ANALOG_POINTER
    char debug_str[128];
    sprintf(debug_str, "Test Results: Basic=%d, Boundary=%d, TIR=%d", 
            (results & 0x0F), 
            (results & 0xF0) >> 4, 
            (results & 0xFF00) >> 8);
    // 添加调试输出
    #endif
}