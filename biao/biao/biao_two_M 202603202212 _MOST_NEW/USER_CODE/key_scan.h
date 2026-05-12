/**
  ******************************************************************************
  * @file           : key_scan.h
  * @brief          : 按键扫描模块头文件
  * @author         : 量博科技
  * @date           : 2026-01-07
  ******************************************************************************
  * @attention
  * 
  * 6键矩阵扫描方案（参考工程优化版）：
  * - PC13, PC14, PC15 组3x2矩阵
  * - LL库直接寄存器操作，无阻塞延时
  * - TIM7定时器中断驱动（50ms周期）
  * - 3态状态机：IDLE→PRESSED→LONG_PRESS
  * - 短按阈值100ms，长按阈值1000ms
  * 
  * 按键定义：
  * - ON/ZERO: 开关机/清零
  * - ↓/ABS/SET: 相对绝对值切换/设置
  * - ↑/IN/MM: 英制公制切换/设置增
  * - TOL: 公差设置
  * - M: 分辨率/MAX/MIN/TIR切换
  * - ABS_ZERO: 原位开关
  * 
  ******************************************************************************
  */

#ifndef __KEY_SCAN_H
#define __KEY_SCAN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum {
    KEY_NONE = 0,
    KEY_ON_ZERO,        // ON/ZERO键
    KEY_DOWN_ABS_SET,   // ↓/ABS/SET键
    KEY_UP_IN_MM,       // ↑/IN/MM键
    KEY_TOL,            // TOL键
    KEY_M,              // M键
    KEY_ABS_ZERO_SW     // ABS原位开关
} KeyCode_t;

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_SHORT_PRESS,  // 短按 (100ms~1000ms)
    KEY_EVENT_LONG_PRESS,   // 长按 (≥2000ms)
    KEY_EVENT_LONG_RELEASE  // 长按后释放
} KeyEvent_t;

typedef struct {
    KeyCode_t   code;
    KeyEvent_t  event;
    uint32_t    timestamp;
} KeyMessage_t;

/* Exported constants --------------------------------------------------------*/
#define KEY_SCAN_INTERVAL_MS        50      // TIM7扫描周期 (ms)
#define KEY_SHORT_PRESS_MIN_COUNT   2       // 短按最小计数 (2×50ms=100ms)
#define KEY_LONG_PRESS_MIN_COUNT    40      // 长按计数阈值 (40×50ms=2000ms)

/* Exported functions prototypes ---------------------------------------------*/
void Key_Init(void);
void Key_TimerScan(void);               // TIM7中断中调用的扫描函数
void Key_Scan(void);                    // 兼容接口，内部调用Key_TimerScan
uint8_t Key_GetMessage(KeyMessage_t *msg);
KeyCode_t Key_MatrixScan(void);         // LL库矩阵扫描
uint8_t Key_IsEngineeringModePressed(void); // 检查是否进入工程模式

#ifdef __cplusplus
}
#endif

#endif /* __KEY_SCAN_H */
