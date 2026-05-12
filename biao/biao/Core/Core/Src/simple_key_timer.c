#include "main.h"
#include <string.h>  // 添加string.h以解决memset函数未声明的警告

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

// 声明htim7变量（如果不存在的话）
extern TIM_HandleTypeDef htim7;
// 简化配置参数
#define KEY_SCAN_INTERVAL_MS      50    // 50ms扫描周期
#define KEY_LONG_PRESS_TIME_MS    1000  // 1000ms长按检测
#define KEY_SCAN_COUNTS_PER_LONG  (KEY_LONG_PRESS_TIME_MS / KEY_SCAN_INTERVAL_MS)  // 20次扫描

// 按键定义（与key_driver.h保持一致）
typedef enum {
    KEY_NONE = 0,           // 无按键
    KEY_ON_ZERO,            // ON/Zero 键
    KEY_DOWN_ABS_SET,       // DOWN/ABS/SET 键
    KEY_UP_IN_MM,           // UP/IN/MM 键
    KEY_TOL,                // TOL 键
    KEY_M,                  // M 键
    KEY_ABS_ZERO_SWITCH     // ABS_ZERO 原位开关
} Key_TypeDef;

// 简化状态结构体
typedef struct {
    uint32_t long_press_counter;
    uint16_t last_key_state;
    uint16_t current_key_state;
    uint32_t long_press_start_time;
} SimpleKeyContext_TypeDef;

// 全局上下文
static SimpleKeyContext_TypeDef g_key_context = {0};

// 按键引脚定义（根据实际硬件配置）
#define KEY_ON_ZERO_PIN          GPIO_PIN_0
#define KEY_ON_ZERO_GPIO_PORT    GPIOA
#define KEY_DOWN_ABS_SET_PIN     GPIO_PIN_1
#define KEY_DOWN_ABS_SET_GPIO_PORT GPIOA
#define KEY_UP_IN_MM_PIN         GPIO_PIN_2
#define KEY_UP_IN_MM_GPIO_PORT   GPIOA
#define KEY_TOL_PIN              GPIO_PIN_3
#define KEY_TOL_GPIO_PORT        GPIOA
#define KEY_M_PIN                GPIO_PIN_4
#define KEY_M_GPIO_PORT          GPIOA
#define KEY_ABS_ZERO_SWITCH_PIN  GPIO_PIN_5
#define KEY_ABS_ZERO_SWITCH_GPIO_PORT GPIOA

// 按键矩阵扫描函数
Key_TypeDef SimpleKey_MatrixScan(void) {
    uint16_t key_state = 0;
    
    // 读取所有按键状态
    if (HAL_GPIO_ReadPin(KEY_ON_ZERO_GPIO_PORT, KEY_ON_ZERO_PIN) == GPIO_PIN_RESET) {
        key_state |= (1 << KEY_ON_ZERO);
    }
    if (HAL_GPIO_ReadPin(KEY_DOWN_ABS_SET_GPIO_PORT, KEY_DOWN_ABS_SET_PIN) == GPIO_PIN_RESET) {
        key_state |= (1 << KEY_DOWN_ABS_SET);
    }
    if (HAL_GPIO_ReadPin(KEY_UP_IN_MM_GPIO_PORT, KEY_UP_IN_MM_PIN) == GPIO_PIN_RESET) {
        key_state |= (1 << KEY_UP_IN_MM);
    }
    if (HAL_GPIO_ReadPin(KEY_TOL_GPIO_PORT, KEY_TOL_PIN) == GPIO_PIN_RESET) {
        key_state |= (1 << KEY_TOL);
    }
    if (HAL_GPIO_ReadPin(KEY_M_GPIO_PORT, KEY_M_PIN) == GPIO_PIN_RESET) {
        key_state |= (1 << KEY_M);
    }
    if (HAL_GPIO_ReadPin(KEY_ABS_ZERO_SWITCH_GPIO_PORT, KEY_ABS_ZERO_SWITCH_PIN) == GPIO_PIN_RESET) {
        key_state |= (1 << KEY_ABS_ZERO_SWITCH);
    }
    
    return (Key_TypeDef)key_state;
}

// 检查是否有任意按键被按下
uint8_t SimpleKey_IsAnyKeyPressed(void) {
    return (SimpleKey_MatrixScan() != KEY_NONE);
}

// 简化定时器按键初始化
bool SimpleKeyTimer_Init(void) {
   // 初始化定时器中断
   HAL_TIM_Base_Start_IT(&htim7);  // 使用htim7变量，修复定时器实例
   // 清除全局上下文
   memset(&g_key_context, 0, sizeof(SimpleKeyContext_TypeDef));
   
   return true;
}

// 开始按键扫描
void SimpleKeyTimer_Start(void) {
   // 启用定时器
   HAL_TIM_Base_Start_IT(&htim7);  // 使用htim7变量，修复定时器实例
}

// 停止按键扫描
void SimpleKeyTimer_Stop(void) {
   // 停止定时器
   HAL_TIM_Base_Stop_IT(&htim7);  // 使用htim7变量，修复定时器实例
}

// 按键扫描主函数
void SimpleKeyTimer_Scan(void) {
   uint16_t current_key_state = (uint16_t)SimpleKey_MatrixScan();
   
   // 更新按键状态
   g_key_context.last_key_state = g_key_context.current_key_state;
   g_key_context.current_key_state = current_key_state;
   
   // 检测按键按下
   if (current_key_state != 0) {
       // 记录长按开始时间
       g_key_context.long_press_start_time = HAL_GetTick();
       g_key_context.long_press_counter = 1;
   } else if (g_key_context.last_key_state != 0) {
       // 按键释放，检查是否为长按
       uint32_t press_duration = HAL_GetTick() - g_key_context.long_press_start_time;
       
       if (press_duration >= KEY_LONG_PRESS_TIME_MS) {
           // 长按事件
           // 可以在这里添加长按处理逻辑
       } else if (g_key_context.long_press_counter >= 1) {
           // 短按事件
           // 可以在这里添加短按处理逻辑
       }
       
       // 重置计数器
       g_key_context.long_press_counter = 0;
   } else {
       // 按键保持按下状态，增加计数器
       if (current_key_state != 0) {
           g_key_context.long_press_counter++;
           
           // 检查是否达到长按阈值
           if (g_key_context.long_press_counter >= KEY_SCAN_COUNTS_PER_LONG) {
               // 长按检测
               // 可以在这里添加长按处理逻辑
           }
       }
   }
}

/* USER CODE END 1 */
