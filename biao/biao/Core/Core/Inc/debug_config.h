#ifndef __DEBUG_CONFIG_H__
#define __DEBUG_CONFIG_H__

/*
 * 调试配置头文件
 * 用于控制系统中的各种调试功能
 */

/* 定义是否启用TIM2调试功能 */
/* #define DEBUG_TIM2 */

/* 定义是否启用传感器采样调试功能 */
/* #define DEBUG_SENSOR_SAMPLING */

/* 定义是否启用按键处理调试功能 */
// #define DEBUG_KEY_PROCESS

/* 定义是否启用LCD显示调试功能 */
// #define DEBUG_LCD_DISPLAY

/* 定义是否启用系统时钟调试功能 */
/* #define DEBUG_SYSTEM_CLOCK */

/* 关闭传感器值调试输出，以减少LCD显示干扰 */
/* #undef DEBUG_SENSOR_VALUE */

/* 启用传感器处理调试 */
// #define DEBUG_SENSOR_PROCESSING

/* 启用按键扫描调试 */
// #define DEBUG_KEY_SCAN

/* 启用传感器值调试 */
// #define DEBUG_SENSOR_VALUE


#endif /* __DEBUG_CONFIG_H__ */
