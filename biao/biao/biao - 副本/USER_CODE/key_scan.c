/**
  ******************************************************************************
  * @file           : key_scan.c
  * @brief          : 按键扫描模块实现（参考工程优化版）
  ******************************************************************************
  * @attention
  * 
  * 优化要点（对照参考工程）：
  * 1. LL库矩阵扫描，无阻塞延时
  * 2. 3态状态机：IDLE→PRESSED→LONG_PRESS
  * 3. TIM7中断驱动50ms周期扫描
  * 4. 短按100ms~1000ms，长按≥1000ms
  * 5. 采样前关TIM7中断，防止与传感器冲突
  * 
  ******************************************************************************
  */

#include "key_scan.h"
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define KEY_MSG_BUFFER_SIZE 8

#define KEY_STATE_IDLE       0
#define KEY_STATE_PRESSED    1
#define KEY_STATE_LONG_PRESS 2

/* Private variables ---------------------------------------------------------*/
static KeyMessage_t key_msg_buffer[KEY_MSG_BUFFER_SIZE];
static uint8_t msg_write_idx = 0;
static uint8_t msg_read_idx = 0;

static uint8_t  key_state = KEY_STATE_IDLE;
static KeyCode_t key_current = KEY_NONE;
static uint32_t key_press_count = 0;       // 按下持续的扫描次数

/* Private function prototypes -----------------------------------------------*/
static void Key_PushMessage(KeyCode_t code, KeyEvent_t event);

/**
  * @brief  按键初始化
  */
void Key_Init(void)
{
    memset(key_msg_buffer, 0, sizeof(key_msg_buffer));
    msg_write_idx = 0;
    msg_read_idx = 0;
    key_state = KEY_STATE_IDLE;
    key_current = KEY_NONE;
    key_press_count = 0;
}

/**
  * @brief  LL库矩阵按键扫描（无阻塞延时）
  * @note   扫描原理：
  *         轮流将一个IO口配置为输出低电平，其他IO口配置为上拉输入
  *         
  *         扫描顺序（与参考工程一致）：
  *         PC15置低，PC13/14上拉 → 检测PC14(ABS_ZERO), PC13(DOWN/ABS/SET)
  *         PC14置低，PC13/15上拉 → 检测PC13(UP/IN/MM), PC15(ON/ZERO)
  *         PC13置低，PC14/15上拉 → 检测PC14(TOL), PC15(M)
  * @retval 检测到的按键码
  */
KeyCode_t Key_MatrixScan(void)
{
    KeyCode_t key_temp = KEY_NONE;
    
    // === 方案1：PC15置低，PC13/14上拉输入 ===
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_15);
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_15);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_14, LL_GPIO_PULL_UP);

    if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_14) == 0) {
        key_temp = KEY_ABS_ZERO_SW;     // PC14低 → ABS_ZERO开关
    } else if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_13) == 0) {
        if (key_temp == KEY_NONE) key_temp = KEY_DOWN_ABS_SET;  // PC13低 → DOWN/ABS/SET
    }
    
    // 恢复PC15为上拉输入
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_15, LL_GPIO_PULL_UP);
    
    // === 方案2：PC14置低，PC13/15上拉输入 ===
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_14);
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_14);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_15, LL_GPIO_PULL_UP);

    if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_13) == 0) {
        if (key_temp == KEY_NONE) key_temp = KEY_UP_IN_MM;      // PC13低 → UP/IN/MM
    } else if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_15) == 0) {
        if (key_temp == KEY_NONE) key_temp = KEY_ON_ZERO;       // PC15低 → ON/ZERO
    }
    
    // 恢复PC14为上拉输入
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_14, LL_GPIO_PULL_UP);
    
    // === 方案3：PC13置低，PC14/15上拉输入 ===
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_13);
    LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_14, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_15, LL_GPIO_PULL_UP);

    if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_14) == 0) {
        if (key_temp == KEY_NONE) key_temp = KEY_TOL;           // PC14低 → TOL
    } else if (LL_GPIO_IsInputPinSet(GPIOC, LL_GPIO_PIN_15) == 0) {
        if (key_temp == KEY_NONE) key_temp = KEY_M;             // PC15低 → M
    }
    
    // 恢复PC13为上拉输入
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_UP);
    
    return key_temp;
}

/**
  * @brief  定时器中断扫描函数（50ms周期，由TIM7中断调用）
  * @note   3态状态机：IDLE → PRESSED → LONG_PRESS
  *         短按：按下100ms~1000ms后释放
  *         长按：按下≥1000ms时立即触发
  */
void Key_TimerScan(void)
{
    KeyCode_t current_key = Key_MatrixScan();
    
    switch (key_state) {
        case KEY_STATE_IDLE:
            if (current_key != KEY_NONE) {
                // 新按键按下，进入PRESSED状态
                key_state = KEY_STATE_PRESSED;
                key_current = current_key;
                key_press_count = 1;
            }
            break;
            
        case KEY_STATE_PRESSED:
            if (current_key == key_current) {
                // 同一按键持续按下
                key_press_count++;
                
                if (key_press_count >= KEY_LONG_PRESS_MIN_COUNT) {
                    // 达到长按阈值，立即触发长按事件
                    Key_PushMessage(key_current, KEY_EVENT_LONG_PRESS);
                    key_state = KEY_STATE_LONG_PRESS;
                }
            } else if (current_key == KEY_NONE) {
                // 按键释放，判断短按
                if (key_press_count >= KEY_SHORT_PRESS_MIN_COUNT &&
                    key_press_count < KEY_LONG_PRESS_MIN_COUNT) {
                    Key_PushMessage(key_current, KEY_EVENT_SHORT_PRESS);
                }
                // 回到空闲
                key_state = KEY_STATE_IDLE;
                key_current = KEY_NONE;
                key_press_count = 0;
            } else {
                // 检测到不同按键，忽略（防抖）
            }
            break;
            
        case KEY_STATE_LONG_PRESS:
            if (current_key != key_current || current_key == KEY_NONE) {
                // 长按后释放
                Key_PushMessage(key_current, KEY_EVENT_LONG_RELEASE);
                key_state = KEY_STATE_IDLE;
                key_current = KEY_NONE;
                key_press_count = 0;
            }
            break;
            
        default:
            key_state = KEY_STATE_IDLE;
            key_current = KEY_NONE;
            key_press_count = 0;
            break;
    }
}

/**
  * @brief  兼容接口，供主循环调用（内部调用Key_TimerScan）
  */
void Key_Scan(void)
{
    Key_TimerScan();
}

/**
  * @brief  压入按键消息
  */
static void Key_PushMessage(KeyCode_t code, KeyEvent_t event)
{
    uint8_t next_idx = (msg_write_idx + 1) % KEY_MSG_BUFFER_SIZE;
    
    if (next_idx != msg_read_idx) {
        key_msg_buffer[msg_write_idx].code = code;
        key_msg_buffer[msg_write_idx].event = event;
        key_msg_buffer[msg_write_idx].timestamp = HAL_GetTick();
        msg_write_idx = next_idx;
    }
}

/**
  * @brief  获取按键消息
  * @retval 1-有消息, 0-无消息
  */
uint8_t Key_GetMessage(KeyMessage_t *msg)
{
    if (msg_read_idx != msg_write_idx) {
        *msg = key_msg_buffer[msg_read_idx];
        msg_read_idx = (msg_read_idx + 1) % KEY_MSG_BUFFER_SIZE;
        return 1;
    }
    return 0;
}
