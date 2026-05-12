#ifndef __SIMPLE_KEY_TIMER_H__
#define __SIMPLE_KEY_TIMER_H

/* Define boolean type for systems that don't support stdbool.h */
#ifndef __cplusplus
#ifndef bool
typedef unsigned char bool;
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

// 简化配置参数
#define KEY_SCAN_INTERVAL_MS      50    // 50ms扫描周期
#define KEY_LONG_PRESS_TIME_MS    1000  // 1000ms长按检测
#define KEY_SCAN_COUNTS_PER_LONG  (KEY_LONG_PRESS_TIME_MS / KEY_SCAN_INTERVAL_MS)  // 20次扫描

// 简化状态结构体
typedef struct {
    uint32_t long_press_counter;
    uint16_t last_key_state;
    uint16_t current_key_state;
    uint32_t long_press_start_time;
} SimpleKeyContext_TypeDef;

// 全局上下文声明
extern SimpleKeyContext_TypeDef g_key_context;

// 简化接口函数声明
bool SimpleKeyTimer_Init(void);
void SimpleKeyTimer_Start(void);
void SimpleKeyTimer_Stop(void);
void SimpleKeyTimer_Scan(void);

#endif /* __SIMPLE_KEY_TIMER_H__ */