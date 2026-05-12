/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    key_driver.c
  * @brief   按键驱动模块实现文件
  * @author  AI
  ******************************************************************************
  * @attention
  *
  * 工作流程：
  * 1. 初始化GPIO引脚和按键状态机
  * 2. 通过矩阵扫描检测按键按下状态
  * 3. 实现软件消抖和长按检测功能
  * 4. 根据按键事件类型（短按/长按）执行相应的处理函数
  * 5. 更新LCD显示和测量状态
  *
  * 实现函数：
  * - KeyDriver_Init(): 按键驱动初始化函数
  * - KeyDriver_Scan(): 按键扫描函数，检测按键状态
  * - KeyDriver_Process(): 按键处理函数，根据按键事件执行相应操作
  * - KeyDriver_GetStatus(): 获取按键状态函数
  * - KeyDriver_MatrixScan(): 按键矩阵扫描函数
  * - KeyDriver_IsAnyKeyPressed(): 检查是否有任意按键被按下函数
  *
  * 该文件实现按键扫描、去抖和事件处理功能
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "gpio.h"
#include "main.h"
#include "global_variables.h"
#include "key_driver.h"
#include "measurement_state_machine.h"
#include "usart.h"  // 添加串口通信支持，用于调试输出
#include <string.h>  // 添加字符串处理函数支持
#include <stdio.h>   // 添加标准输入输出支持
#include "lcd_display.h"  // 添加LCD显示支持，用于更新显示

/* Define DEBUG_KEY_SCAN to enable detailed key scan debugging */
#ifndef DEBUG_KEY_SCAN
#define DEBUG_KEY_SCAN
#endif

/* Define DEBUG_KEY_SCAN to enable detailed key scan debugging */
/* #ifndef DEBUG_KEY_SCAN
#define DEBUG_KEY_SCAN
#endif */

// 删除此函数，因为我们不再需要计算时间差

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* 按键扫描相关定义 */
#define KEY_LONG_PRESS_TIME_MS    1000

/* 简化定时器按键相关定义 */
#define KEY_SCAN_INTERVAL_MS      50    // 50ms扫描周期
#define KEY_SCAN_COUNTS_PER_LONG  (KEY_LONG_PRESS_TIME_MS / KEY_SCAN_INTERVAL_MS)  // 20次扫描

/* 按键GPIO定义 - 根据项目需求调整 */
#define KEY_ROW_1_PIN             LL_GPIO_PIN_13  // PC13
#define KEY_ROW_2_PIN             LL_GPIO_PIN_14  // PC14
#define KEY_ROW_3_PIN             LL_GPIO_PIN_15  // PC15
#define KEY_COL_1_PIN             LL_GPIO_PIN_14  // PC14 (when row 1 is low)
#define KEY_COL_2_PIN             LL_GPIO_PIN_13  // PC13 (when row 1 is low)
#define KEY_COL_3_PIN             LL_GPIO_PIN_15  // PC13 (when row 1 is low)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

// 声明外部函数
extern void Data_Filter_Reset(int32_t reset_value);

/* 定时器相关上下文结构体 */
typedef struct {
    uint32_t long_press_counter;
    uint16_t last_key_state;
    uint16_t current_key_state;
    uint32_t long_press_start_time;
} KeyTimerContext_TypeDef;

/* 内部按键状态管理 */
typedef enum {
    KEY_INTERNAL_STATE_IDLE = 0,        // 空闲状态
    KEY_INTERNAL_STATE_PRESSED,         // 按下状态
    KEY_INTERNAL_STATE_LONG_PRESS       // 长按状态
} KeyInternalState_t;

typedef struct {
    KeyInternalState_t state;      // 当前内部状态
    Key_TypeDef key_type;          // 当前按键类型
} InternalKeyState_t;

static InternalKeyState_t internal_key_state = {
    .state = KEY_INTERNAL_STATE_IDLE,
    .key_type = KEY_NONE
};

// 记录按键按下时的TIM7中断次数
static uint32_t key_press_start_interrupt_count = 0;

// 定时器上下文
static KeyTimerContext_TypeDef key_timer_context = {0};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* Function to check if any key is pressed */
uint8_t KeyDriver_IsAnyKeyPressed(void);

/* 定时器相关函数声明 */
bool KeyDriver_Timer_Init(void);
void KeyDriver_Timer_Start(void);
void KeyDriver_Timer_Stop(void);
void KeyDriver_Timer_Scan(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  按键驱动初始化
  * @param  None
  * @retval None
  */
void KeyDriver_Init(void)
{
    // 初始化GPIO - 已在MX_GPIO_Init中完成配置
    // PC13, PC14, PC15 配置为输入，带内部上拉
    
    // 初始化全局按键状态机
    key_fsm.value = 0;  // 清零所有状态位
    
    // 初始化内部按键状态
    internal_key_state.state = KEY_INTERNAL_STATE_IDLE;
    internal_key_state.key_type = KEY_NONE;
    
    // 初始化定时器上下文
    memset(&key_timer_context, 0, sizeof(KeyTimerContext_TypeDef));
}

/**
  * @brief  按键扫描函数
  * @param  None
  * @retval 按键扫描结果
  */
KeyScanResult_TypeDef KeyDriver_Scan(void)
{
    KeyScanResult_TypeDef result = {KEY_NONE, KEY_EVENT_NONE, false};
    
    Key_TypeDef current_key = KeyDriver_MatrixScan();
    
    // 获取当前TIM7中断次数
    extern uint32_t tim7_interrupt_count; // 假设在某处定义了全局中断计数器
    uint32_t current_interrupt_count = tim7_interrupt_count;
    
    // 状态机处理逻辑
    switch(internal_key_state.state)
    {
        case KEY_INTERNAL_STATE_IDLE:
            if(current_key != KEY_NONE)
            {
                // 进入按下状态
                internal_key_state.state = KEY_INTERNAL_STATE_PRESSED;
                internal_key_state.key_type = current_key;
                // 记录按下时的TIM7中断次数
                key_press_start_interrupt_count = current_interrupt_count;
                
                #ifdef DEBUG_KEY_SCAN
                char debug_msg[50];
                sprintf(debug_msg, "Key Pressed: %d at interrupt %lu", current_key, current_interrupt_count);
                // 可以添加串口输出或其他调试输出
                #endif
            }
            break;
            
        case KEY_INTERNAL_STATE_PRESSED:
            if(current_key == internal_key_state.key_type)
            {
                // 按键仍然按下，检查是否达到长按阈值
                // 计算按下时间（单位：ms）
                // TIM7每1ms产生一次中断，所以中断次数差就是按下的毫秒数
                
                // 处理中断计数器溢出（32位计数器，最大值0xFFFFFFFF）
                uint32_t press_time;
                if (current_interrupt_count >= key_press_start_interrupt_count) {
                    // 未溢出情况
                    press_time = current_interrupt_count - key_press_start_interrupt_count;
                } else {
                    // 溢出情况
                    // 假设中断计数器是32位，最大值0xFFFFFFFF
                    press_time = (0xFFFFFFFF - key_press_start_interrupt_count) + current_interrupt_count + 1;
                }
                
                // 长按阈值设为1000ms
                if(press_time >= KEY_LONG_PRESS_TIME_MS)
                {
                    // 进入长按状态
                    internal_key_state.state = KEY_INTERNAL_STATE_LONG_PRESS;
                    
                    // 设置对应按键的长按事件标志
                    switch(internal_key_state.key_type)
                    {
                        case KEY_TOL:
                            key_event_flags.bits.l_tol_evt = 1;
                            break;
                        case KEY_M:
                            key_event_flags.bits.l_m_evt = 1;
                            break;
                        case KEY_UP_IN_MM:
                            key_event_flags.bits.l_up_in_mm_evt = 1;
                            break;
                        case KEY_DOWN_ABS_SET:
                            key_event_flags.bits.l_down_abs_set_evt = 1;
                            break;
                        case KEY_ON_ZERO:
                            key_event_flags.bits.l_on_zero_evt = 1;
                            break;
                        case KEY_ABS_ZERO_SWITCH:
                            key_event_flags.bits.l_abszero_evt = 1;
                            break;
                        default:
                            break;
                    }
                    
                    // 生成长按事件
                    result.key_type = internal_key_state.key_type;
                    result.event = KEY_EVENT_LONG_PRESS;
                    result.is_pressed = true;
                    
                    #ifdef DEBUG_KEY_SCAN
                    char debug_msg[50];
                    sprintf(debug_msg, "Long Press Detected: %d", internal_key_state.key_type);
                    // 可以添加串口输出或其他调试输出
                    #endif
                }
                // 按键仍然按下，不做其他处理，等待释放或达到长按阈值
            }
            else if(current_key == KEY_NONE)
            {
                // 按键释放，判断是短按还是快速释放
                // 计算按下时间
                // 处理中断计数器溢出（32位计数器，最大值0xFFFFFFFF）
                uint32_t press_time;
                if (current_interrupt_count >= key_press_start_interrupt_count) {
                    // 未溢出情况
                    press_time = current_interrupt_count - key_press_start_interrupt_count;
                } else {
                    // 溢出情况
                    // 假设中断计数器是32位，最大值0xFFFFFFFF
                    press_time = (0xFFFFFFFF - key_press_start_interrupt_count) + current_interrupt_count + 1;
                }
                
                // 只处理短按情况（在达到长按阈值前释放）
                if(press_time >= 10 && press_time < KEY_LONG_PRESS_TIME_MS)  // 10ms-1000ms之间的按键，视为有效短按
                
                {
                    
                    // 将1-10次循环的按键视为短按处理，避免按键值丢失
                    switch(internal_key_state.key_type)
                    {
                        case KEY_TOL:
                            key_event_flags.bits.s_tol_evt = 1;
                            break;
                        case KEY_M:
                            key_event_flags.bits.s_m_evt = 1;
                            break;
                        case KEY_UP_IN_MM:
                            key_event_flags.bits.s_up_in_mm_evt = 1;
                            break;
                        case KEY_DOWN_ABS_SET:
                            key_event_flags.bits.s_down_abs_set_evt = 1;
                            break;
                        case KEY_ON_ZERO:
                            key_event_flags.bits.s_on_zero_evt = 1;
                            break;
                        case KEY_ABS_ZERO_SWITCH:
                            key_event_flags.bits.s_abszero_evt = 1;
                            break;
                        default:
                            break;
                    }
                    
                    // 生成短按事件
                    result.key_type = internal_key_state.key_type;
                    result.event = KEY_EVENT_SHORT_PRESS;
                    result.is_pressed = true;
                    
                    #ifdef DEBUG_KEY_SCAN
                    char debug_msg[50];
                    sprintf(debug_msg, "Short Press Detected: %d (duration: %lu ms)", internal_key_state.key_type, press_time);
                    // 可以添加串口输出或其他调试输出
                    #endif
                }
                // 按键释放，重置状态
                // 注意：不再在这里清除key_fsm中的状态位，因为这些状态需要被显示函数使用
                // key_fsm中的状态位将在事件被处理后由相应的处理函数清除
                
                // 回到空闲状态
                internal_key_state.state = KEY_INTERNAL_STATE_IDLE;
                internal_key_state.key_type = KEY_NONE;
            }
            else
            {
                // 检测到不同的按键按下，可能是误触或按键干扰
                // 忽略新按键，保持当前按键状态
                // 这样可以避免按键干扰，确保当前按键事件完成
            }
            break;
            
        case KEY_INTERNAL_STATE_LONG_PRESS:
            if(current_key != internal_key_state.key_type && current_key == KEY_NONE)
            {
                // 按键释放，这是长按后的释放
                // 注意：不再在这里清除key_fsm中的状态位，因为这些状态需要被显示函数使用
                // key_fsm中的状态位将在事件被处理后由相应的处理函数清除

                // 生成长按释放事件（使用相同的key_type，但event为NONE表示释放）
                result.key_type = internal_key_state.key_type;
                result.event = KEY_EVENT_NONE;  // 表示按键释放，但已知之前发生了长按
                result.is_pressed = false;
                
                #ifdef DEBUG_KEY_SCAN
                char debug_msg[50];
                sprintf(debug_msg, "Long Press Released: %d", internal_key_state.key_type);
                // 可以添加串口输出或其他调试输出
                #endif
                
                // 回到空闲状态
                internal_key_state.state = KEY_INTERNAL_STATE_IDLE;
                internal_key_state.key_type = KEY_NONE;
            }
            // 如果按键仍然按下或检测到其他按键，继续保持长按状态，不做额外处理
            break;
            
        default:
            // 如果状态异常，回到空闲状态
            internal_key_state.state = KEY_INTERNAL_STATE_IDLE;
            internal_key_state.key_type = KEY_NONE;
            break;
    }
    
    return result;
}

/**
  * @brief  获取按键状态
  * @param  key: 按键类型
  * @retval 按键状态
  */
uint8_t KeyDriver_GetStatus(Key_TypeDef key)
{
    if(internal_key_state.key_type == key && (internal_key_state.state == KEY_INTERNAL_STATE_PRESSED || internal_key_state.state == KEY_INTERNAL_STATE_LONG_PRESS))
    {
        return 1; // 按键按下
    }
    return 0; // 按键释放
}


/**
  * @brief  按键矩阵扫描
  * @param  None
  * @retval 按键类型
  *
  * 按键矩阵连接方式（根据README.md描述）：
  * Pc13,14内部强上拉，PC15置低，检测pc14为低是ABS_zero原位开关按下，pc13为低是↓ / ABS / SET键按下
  * PC13,15内部强上拉，PC14置低，检测pc13为低是↑ / IN / MM按下，pc15为低是on/zero键按下
  * PC14,15内部强上拉，PC13置低，检测pc14为低是TOL按下，pc15为低是M键按下
  */
Key_TypeDef KeyDriver_MatrixScan(void)
{
    Key_TypeDef key_temp = KEY_NONE;
    
    // 方案1：PC15置低，PC13,PC14内部强上拉
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_3_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(GPIOC, KEY_ROW_3_PIN);  // 先置高
    LL_GPIO_ResetOutputPin(GPIOC, KEY_ROW_3_PIN); // 置低
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_1_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_1_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_2_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_2_PIN, LL_GPIO_PULL_UP);

    // 检测按键
    if(LL_GPIO_IsInputPinSet(GPIOC, KEY_ROW_2_PIN) == 0) // PC14为低 -> ABS_ZERO开关
    {
        key_temp = KEY_ABS_ZERO_SWITCH;
    }
    else if(LL_GPIO_IsInputPinSet(GPIOC, KEY_ROW_1_PIN) == 0) // PC13为低 -> DOWN/ABS/SET
    {
        if(key_temp == KEY_NONE) key_temp = KEY_DOWN_ABS_SET;
    }
    
    // 恢复GPIO配置
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_3_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_3_PIN, LL_GPIO_PULL_UP);  // 修改为带上拉，保持一致性
    
    // 方案2：PC14置低，PC13,PC15内部强上拉
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_2_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(GPIOC, KEY_ROW_2_PIN);  // 先置高
    LL_GPIO_ResetOutputPin(GPIOC, KEY_ROW_2_PIN); // 置低
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_1_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_1_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_3_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_3_PIN, LL_GPIO_PULL_UP);

    // 检测按键
    if(LL_GPIO_IsInputPinSet(GPIOC, KEY_ROW_1_PIN) == 0) // PC13为低 -> UP/IN/MM
    {
        if(key_temp == KEY_NONE) key_temp = KEY_UP_IN_MM;
    }
    else if(LL_GPIO_IsInputPinSet(GPIOC, KEY_ROW_3_PIN) == 0) // PC15为低 -> ON/ZERO
    {
        if(key_temp == KEY_NONE) key_temp = KEY_ON_ZERO;
    }
    
    // 恢复GPIO配置
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_2_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_2_PIN, LL_GPIO_PULL_UP);  // 修改为带上拉，保持一致性
    
    // 方案3：PC13置低，PC14,PC15内部强上拉
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_1_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetOutputPin(GPIOC, KEY_ROW_1_PIN);  // 先置高
    LL_GPIO_ResetOutputPin(GPIOC, KEY_ROW_1_PIN); // 置低
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_2_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_2_PIN, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_3_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_3_PIN, LL_GPIO_PULL_UP);

    // 检测按键
    if(LL_GPIO_IsInputPinSet(GPIOC, KEY_ROW_2_PIN) == 0) // PC14为低 -> TOL
    {
        if(key_temp == KEY_NONE) key_temp = KEY_TOL;
    }
    else if(LL_GPIO_IsInputPinSet(GPIOC, KEY_ROW_3_PIN) == 0) // PC15为低 -> M
    {
        if(key_temp == KEY_NONE) key_temp = KEY_M;
    }
    
    // 恢复GPIO配置
    LL_GPIO_SetPinMode(GPIOC, KEY_ROW_1_PIN, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(GPIOC, KEY_ROW_1_PIN, LL_GPIO_PULL_UP);  // 修改为带上拉，保持一致性
    
    #ifdef DEBUG_KEY_SCAN
    if(key_temp != KEY_NONE) {
        char debug_msg[30];
        sprintf(debug_msg, "Key Scanned: %d", key_temp);
        // 可以添加串口输出或其他调试输出
    }
    #endif
    
    return key_temp;
}

/**
 * @brief  按键处理函数
 * @param  result: 按键扫描结果指针
 * @retval None
 */
__attribute__((used)) void KeyDriver_Process(KeyScanResult_TypeDef *result)
{
   if(result == NULL) return;

   // 根据按键事件类型进行相应处理
   switch(result->event)
   {
       case KEY_EVENT_SHORT_PRESS:
           // 短按处理
           {
               uint8_t current_mode = ZTJ_GetCurrentMode(); // 在switch之前声明变量
               
               switch(result->key_type)
               {
                   case KEY_TOL:
                       // TOL键短按处理 - 在ABS/REL模式中切换公差检查状态
                       
                       if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
                           // 在公差设置模式下，短按TOL键切换设置步骤（上公差/下公差）
                           ZTJ_ToggleToleranceSettingStep();
                       } else if(measurement_state.mode == MEASUREMENT_MODE_ABS || measurement_state.mode == MEASUREMENT_MODE_REL) {
                           // 在绝对模式和相对模式下，短按TOL键切换公差检查状态（开启/关闭公差判断）
                           ZTJ_ToggleToleranceCheck();
                       }
                       // 在其他模式下，TOL键不执行任何操作
                       break;
                   case KEY_M:
                       // M键短按处理 - 在公差设置模式下退出设置，否则循环切换MAX/MIN/TIR模式
                       
                       if(current_mode == MEASUREMENT_MODE_TOL_SETTING) {
                           // 在公差设置模式下，短按M键退出设置模式，回到NORMAL测量模式
                           measurement_state.mode = MEASUREMENT_MODE_NORMAL;
                       } else if(current_mode == MEASUREMENT_MODE_REL ||
                          current_mode == MEASUREMENT_MODE_MAX ||
                          current_mode == MEASUREMENT_MODE_MIN ||
                          current_mode == MEASUREMENT_MODE_TIR) {
                           // 在REL、MAX、MIN、TIR模式下允许循环切换
                           ZTJ_CycleMaxMinTir();
                       }
                       // 在NORMAL、ABS等其他模式下，M键不执行任何操作
                       break;
                   case KEY_UP_IN_MM:
                       // UP/IN/MM键短按处理 - 在公差设置模式下调整公差值，否则切换单位 (MM/IN)
                       
                       if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
                           // 在公差设置模式下，UP键增加公差值
                           ZTJ_HandleToleranceSetting(1);  // 增加公差值
                           
                           // 立即更新显示以反映公差值变化
                           extern SensorData_TypeDef processed_data;
                           LCDDisplay_ShowProcessedData(processed_data);
                       } else {
                           // 非公差设置模式下，切换单位 (MM/IN)
                           
                           LCDDisplay_ShowString_New("UI");  // 显示UP/IN/MM按键响应
                           LCDDisplay_Update_New();
                           HAL_Delay(100);  // 短暂显示
                          
                           
                           // 获取当前测量状态
                           ZTJ_TypeDef* current_measure_state = ZTJ_GetCurrentState();
                           
                           // 记录单位切换前的状态
                           uint8_t old_unit = current_measure_state->unit;
                           uint8_t current_is_metric;
                           uint8_t is_metric_value;
                           
                           ZTJ_ToggleUnit();
                           
                           // 如果单位发生了变化，更新LCD显示状态机并重新处理当前传感器数据以更新显示
                           if (old_unit != current_measure_state->unit) {
                               
                               // 更新LCD显示状态机的单位设置，使用布尔值表示是否为公制
                               extern void LCD_DisplayState_SetMetric(uint8_t is_metric);
                               // 将单位值转换为布尔值：0(MM)=true(公制)，1(IN)=false(英制)
                               is_metric_value = current_measure_state->unit == 0 ? 1 : 0;
                               LCD_DisplayState_SetMetric(is_metric_value);
                               
                               // 重新处理当前数据以确保单位转换生效
                               extern SensorData_TypeDef processed_data;
                               extern SensorData_TypeDef DataProcessing_Process(int32_t raw_value);
                               extern volatile Sample_sample_t current_sample;
                               
                               // 使用当前传感器值重新处理数据，确保单位转换生效
                               if (current_sample.value != 0) { // 增加数据有效性检查
                                   processed_data = DataProcessing_Process(current_sample.value);
                                   
                                   // 增加LCD显示保护，防止死机
                                   uint32_t lcd_timeout_start = HAL_GetTick();
                                   while (HAL_GetTick() - lcd_timeout_start < 100) { // 100ms超时保护
                                       // 简单的LCD显示调用，避免复杂操作
                                       if (processed_data.is_valid) {
                                           LCDDisplay_ShowProcessedData(processed_data);
                                           break;
                                       }
                                   }
                               }
                           }
                           
                           // 确保按键处理后，LCD显示状态机中的单位与测量状态机保持一致
                           extern void LCD_DisplayState_SetMetric(uint8_t is_metric);
                           current_is_metric = measurement_state.unit == 0 ? 1 : 0;
                           LCD_DisplayState_SetMetric(current_is_metric);
                       }
                       break;
                   case KEY_DOWN_ABS_SET:
                       // DOWN/ABS/SET键短按处理 - 在公差设置模式下调整公差值，否则按照用户数据处理流程：短按ABS后，Rer_bk_data=Abs_data记录相对基准点，Rer_data=Rew_data-Rer_bk_data
                       
                       if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
                           // 在公差设置模式下，DOWN键减少公差值
                           ZTJ_HandleToleranceSetting(-1); // 减少公差值
                           
                           // 立即更新显示以反映公差值变化
                           extern SensorData_TypeDef processed_data;
                           LCDDisplay_ShowProcessedData(processed_data);
                       } else {
                           if(current_mode == MEASUREMENT_MODE_ABS)
                           {
                               // 按照用户流程：短按ABS后，Rer_bk_data=Abs_data记录相对基准点
                               // 调用数据处理函数记录当前绝对值作为相对基准
                               DataProcessing_SetRelBaselineFromAbs();
                               
                               // 从绝对模式切换到相对模式
                               ZTJ_SwitchAbsToRel();
                           }
                           else
                           {
                               // 从相对模式切换到绝对模式
                               ZTJ_SwitchRelToAbs();
                           }
                       }
                       break;
                   case KEY_ON_ZERO:
                       // ON/ZERO键短按处理 - 按照用户数据处理流程：短按ZERO后，Abs_bk_data=Rew_data记录基准点，Abs_data=Rew_data-Abs_bk_data
                       
                       // 在清零前，先重置滤波器以确保清零准确性
                       Data_Filter_Reset(0);
                       
                       // 在任何模式下都执行清零操作，根据用户描述的逻辑：
                       // 在绝对模式下：abs_bk_data=rew_data, abs_data=rew_data-abs_bk_data
                       // 进入相对模式时：rel_bk_data=abs_data, rel_data=abs_data-rel_bk_data
                       extern volatile Sample_sample_t current_sample;  // 声明外部变量
                       
                       if(current_mode == MEASUREMENT_MODE_ABS)
                       {
                           // 在绝对模式下，设置标志，表示需要在下一次数据处理时使用最新的滤波数据作为基准点
                           // 这样可以确保使用的是滤波后的Rew_data，而不是原始的current_sample.value
                           
                           // 设置需要手动清零校准的全局标志，将在下一次数据处理时使用滤波后的数据
                           extern uint8_t manual_zero_calibration_requested;
                           manual_zero_calibration_requested = 1;
                           
                           // 只更新测量状态机中的绝对零点偏移
                           measurement_state.absolute_zero_offset = current_sample.value;
                       }
                       else if(current_mode == MEASUREMENT_MODE_REL)
                       {
                           // 在相对模式下，应该更新相对基准点，而不是绝对基准点
                           // 需要调用数据处理函数来更新相对基准点
                           extern void DataProcessing_SetRelBaselineFromAbs(void);
                           
                           // 在相对模式下，不设置manual_zero_calibration_requested标志，避免更新abs_bk_data
                           // 只更新相对基准点
                           DataProcessing_SetRelBaselineFromAbs();
                           
                           // 在相对模式下，也将当前值设为零点偏移
                           measurement_state.zero_offset = current_sample.value;
                           
                           // 同时更新测量状态机中的相对基准点
                           measurement_state.rer_bk_data = current_sample.value;
                           
                           // 重置滤波器，使用当前传感器值初始化滤波器
                           Data_Filter_Reset(current_sample.value);
                       }
                       else
                       {
                           // 在其他模式下（如NORMAL、MAX、MIN、TIR等），执行普通归零操作
                           // 将当前值设为零点偏移，实现归零功能
                           measurement_state.zero_offset = current_sample.value;
                           
                           // 无论当前是什么模式，按zero键都应该设置手动清零校准标志，
                           // 以确保在下次数据处理时更新abs_bk_data，这样切换到绝对模式时才能正确显示
                           extern uint8_t manual_zero_calibration_requested;
                           manual_zero_calibration_requested = 1;
                       }

                       // 重置滤波器，使用当前传感器值初始化滤波器
                       Data_Filter_Reset(current_sample.value);
                       
                       // 输出零点脉冲到PB2
                      // OutputZeroPulseOnPB2();
                       
                       // 在清零操作后，确保显示值为0
                       extern SensorData_TypeDef processed_data;
                       extern SensorData_TypeDef DataProcessing_Process(int32_t raw_value);
                       extern volatile Sample_sample_t current_sample;  // 声明外部变量
                       
                       // 在清零操作后，为了确保显示为0，我们需要重新处理当前传感器值
                       // 这样可以确保应用了新的零点偏移
                       // processed_data = DataProcessing_Process(current_sample.value); // 使用当前传感器值重新处理
                       
                       // 清零后应该显示0，但应通过正常的数据处理流程实现，而不是直接赋值
                       // processed_data.value = 0;
                       // processed_data.sign = 0; // 0没有正负之分
                       
                       // 设置数字显示为0
                       // for(uint8_t i = 0; i < 6; i++) {
                       //     processed_data.digits[i] = 0;
                       // }
                       
                       // 让数据处理函数处理显示值，而不是直接修改
                       LCDDisplay_ShowProcessedData(processed_data);
                       
                       // 添加调试日志，确认清零操作已执行
                       #ifdef DEBUG_KEY_SCAN
                       char debug_msg[30];
                       sprintf(debug_msg, "Zero Calibration Done");
                       // 可以添加串口输出或其他调试输出
                       #endif
                       break;
                   case KEY_ABS_ZERO_SWITCH:
                       // ABS_ZERO开关短按处理 - 可用于特殊功能
                       break;
                   default:
                       break;
               } // 结束内部switch语句
           }
           break;
           
       case KEY_EVENT_LONG_PRESS:
           // 长按处理
           
           switch(result->key_type)
           {
               case KEY_TOL:
                   // TOL键长按处理 - 进入公差值设置模式，分别设置TOL_MAX(正公差)TOL_Min(负公差)
                   measurement_state.mode = MEASUREMENT_MODE_TOL_SETTING;
                   
                   // 在公差设置模式下，公差值与当前测量值无关，保持原有的公差值不变
                   // 不改变现有的上公差和下公差值
                   
                   ZTJ_UpdateLastActivity();
                   break;
               case KEY_M:
                   // M键长按处理 - 循环切换分辨率模式 (0.5u, 1u, 0.01mm)
                   ZTJ_CycleResolutionMode();
                   break;
               case KEY_UP_IN_MM:
                   // UP/IN/MM键长按处理 - 在公差设置模式下调整公差值，否则切换单位
                   if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
                       // 在公差设置模式下，长按UP键增加公差值
                       ZTJ_HandleToleranceSetting(1);  // 增加公差值
                       
                       // 立即更新显示以反映公差值变化
                       extern void LCDDisplay_ShowProcessedData(SensorData_TypeDef data);
                       extern SensorData_TypeDef processed_data;
                       LCDDisplay_ShowProcessedData(processed_data);
                   } else {
                       // 非公差设置模式下，切换单位
                       ZTJ_ToggleUnit();
                   }
                   break;
               case KEY_DOWN_ABS_SET:
                   // DOWN/ABS/SET键长按处理 - 在公差设置模式下调整公差值，否则进入设置模式
                   if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
                       // 在公差设置模式下，长按DOWN键减少公差值
                       ZTJ_HandleToleranceSetting(-1); // 减少公差值
                       
                       // 立即更新显示以反映公差值变化
                       extern SensorData_TypeDef processed_data;
                       LCDDisplay_ShowProcessedData(processed_data);
                   } else {
                       // 非公差设置模式下，进入设置模式
                       ZTJ_EnterSetupMode();
                   }
                   break;
               case KEY_ON_ZERO:
                   // ON/ZERO键长按处理 - 开关机
                   ZTJ_TogglePower(ZTJ_GetCurrentState());
                   break;
               case KEY_ABS_ZERO_SWITCH:
                   // ABS_ZERO开关长按处理
                   break;
               default:
                   break;
           }
           break;
           
       default:
           #ifdef DEBUG_KEY_PROCESS
           LCDDisplay_ShowString_New("NO");  // 显示无事件
           LCDDisplay_Update_New();
           HAL_Delay(100);  // 短暂显示
           #endif
           break;
   }

   // 更新按键响应显示状态
   if(result->event != KEY_EVENT_NONE) {
       is_in_key_response = 1;  // 开始按键响应显示
       key_response_end_time = HAL_GetTick() + 500;  // 500ms后结束显示
       
       // 更新最后活动时间以防止自动关机
       ZTJ_UpdateLastActivity();
   }
   
   // 处理按键事件标志并更新显示缓存，实现状态机统一管理
   if(key_event_flags.bits.s_tol_evt) {
       // 更新LCD显示缓存 - TOL短按
       LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);  // 显示TOL图标
       key_fsm.bits.s_tol = 1;  // 同时更新状态机状态
       key_event_flags.bits.s_tol_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.l_tol_evt) {
       // 更新LCD显示缓存 - TOL长按
       LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);  // 显示TOL图标
       key_fsm.bits.l_tol = 1;  // 同时更新状态机状态
       key_event_flags.bits.l_tol_evt = 0;  // 清除事件标志
       
       // 如果当前处于公差设置模式，处理公差设置
       if(measurement_state.mode == MEASUREMENT_MODE_TOL_SETTING) {
           // 在公差设置模式下，根据当前设置步骤显示相应的图标闪烁
           // 使用主循环计数器实现每10个主循环翻转一次的闪烁效果
           if(measurement_state.tolerance_setting_step == 0) {
               // 正在设置上公差（正公差），显示tol+MAX图标闪烁
               if((LOOP_tol_LCD % 20) < 10) {  // 每20个循环闪烁一次，前10个循环点亮，后10个循环熄灭
                   LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);  // 显示TOL图标
                   LCDDisplay_ShowIcon_New(LCD_ICON_MAX, 1);  // 显示MAX图标
               } else {
                   LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 0);  // 关闭TOL图标
                   LCDDisplay_ShowIcon_New(LCD_ICON_MAX, 0);  // 关闭MAX图标
               }
           } else if(measurement_state.tolerance_setting_step == 1) {
               // 正在设置下公差（负公差），显示tol+MIN图标闪烁
               if((LOOP_tol_LCD % 20) < 10) {  // 每20个循环闪烁一次，前10个循环点亮，后10个循环熄灭
                   LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 1);  // 显示TOL图标
                   LCDDisplay_ShowIcon_New(LCD_ICON_MIN, 1);  // 显示MIN图标
               } else {
                   LCDDisplay_ShowIcon_New(LCD_ICON_TOL, 0);  // 关闭TOL图标
                   LCDDisplay_ShowIcon_New(LCD_ICON_MIN, 0);  // 关闭MIN图标
               }
           }

           // 增加主循环计数器
           LOOP_tol_LCD++;
       }
       // 注意：公差值的调整已经在短按和长按事件处理中完成，这里不再重复处理
   }
   if(key_event_flags.bits.s_m_evt) {
       // 更新LCD显示缓存 - M短按
       #ifdef DEBUG_KEY_PROCESS
       LCDDisplay_ShowIcon_New(LCD_ICON_M, 1);  // 显示M图标
       #endif
       key_fsm.bits.s_m = 1;  // 更新状态机状态
       key_event_flags.bits.s_m_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.l_m_evt) {
       // 更新LCD显示缓存 - M长按
       key_fsm.bits.l_m = 1;  // 更新状态机状态
       key_event_flags.bits.l_m_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.s_up_in_mm_evt) {
       // 更新LCD显示缓存 - UP/IN/MM短按
       key_fsm.bits.s_up_in_mm = 1;  // 更新状态机状态
       key_event_flags.bits.s_up_in_mm_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.l_up_in_mm_evt) {
       // 更新LCD显示缓存 - UP/IN/MM长按
       key_fsm.bits.l_up_in_mm = 1; // 更新状态机状态
       key_event_flags.bits.l_up_in_mm_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.s_down_abs_set_evt) {
       // 更新LCD显示缓存 - DOWN/ABS/SET短按
       key_fsm.bits.s_down_abs_set = 1;  // 更新状态机状态
       key_event_flags.bits.s_down_abs_set_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.l_down_abs_set_evt) {
       // 更新LCD显示缓存 - DOWN/ABS/SET长按
       key_fsm.bits.l_down_abs_set = 1;  // 更新状态机状态
       key_event_flags.bits.l_down_abs_set_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.s_on_zero_evt) {
       // 更新LCD显示缓存 - ON/ZERO短按
       key_fsm.bits.s_on_zero = 1;  // 更新状态机状态
       key_event_flags.bits.s_on_zero_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.l_on_zero_evt) {
       // 更新LCD显示缓存 - ON/ZERO长按
       key_fsm.bits.l_on_zero = 1;  // 更新状态机状态
       key_event_flags.bits.l_on_zero_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.s_abszero_evt) {
       // 更新LCD显示缓存 - ABS ZERO短按
       key_fsm.bits.s_abszero = 1;  // 更新状态机状态
       key_event_flags.bits.s_abszero_evt = 0;  // 清除事件标志
   }
   if(key_event_flags.bits.l_abszero_evt) {
       // 更新LCD显示缓存 - ABS ZERO长按
       key_fsm.bits.l_abszero = 1;  // 更新状态机状态
       key_event_flags.bits.l_abszero_evt = 0;  // 清除事件标志
   }
   
   // 如果是长按M键改变分辨率，则需要重新处理数据以反映新的分辨率
   if(result->key_type == KEY_M && result->event == KEY_EVENT_LONG_PRESS) {
       extern SensorData_TypeDef processed_data;
       extern SensorData_TypeDef DataProcessing_Process(int32_t raw_value);
       // 使用当前的raw_value重新处理数据，以应用新的分辨率设置
       processed_data = DataProcessing_Process(current_sample.value);
   }

   // 统一更新LCD显示
   extern SensorData_TypeDef processed_data;
   LCDDisplay_ShowProcessedData(processed_data);
   
   // 特别处理：当分辨率改变后，需要强制更新显示以确保新模式正确显示
   if(result->key_type == KEY_M && result->event == KEY_EVENT_LONG_PRESS) {
       // 延迟一点时间再次更新显示，确保分辨率变化完全生效
       HAL_Delay(50); // 短暂延迟，让系统稳定
       LCDDisplay_ShowProcessedData(processed_data); // 再次更新显示
   }
   
   #ifdef DEBUG_KEY_SCAN
   char debug_msg[50];
   sprintf(debug_msg, "Key Processed: %d, Event: %d", result->key_type, result->event);
   // 可以添加串口输出或其他调试输出
   #endif
}

/* USER CODE END 0 */

/* 定时器相关函数实现 */

// 简化定时器按键初始化
bool KeyDriver_Timer_Init(void) {
   // 初始化定时器中断
   // 注意：htim7需要在main.c中定义和初始化
   // 这里只是设置上下文
   memset(&key_timer_context, 0, sizeof(KeyTimerContext_TypeDef));
   
   return true;
}

// 启动按键扫描定时器
void KeyDriver_Timer_Start(void) {
   // 启用定时器
   HAL_TIM_Base_Start_IT(&htim7);  // 使用htim7变量，修复定时器实例
}

// 停止按键扫描定时器
void KeyDriver_Timer_Stop(void) {
   // 停止定时器
   HAL_TIM_Base_Stop_IT(&htim7);  // 使用htim7变量，修复定时器实例
}

// 按键扫描主函数
void KeyDriver_Timer_Scan(void) {
   uint16_t current_key_state = (uint16_t)KeyDriver_MatrixScan();
   
   // 更新按键状态
   uint16_t prev_key_state = key_timer_context.current_key_state;
   key_timer_context.current_key_state = current_key_state;
   
   // 检测按键状态变化
   if (current_key_state != 0) {
       // 有按键按下
       if (prev_key_state == 0) {
           // 新按键按下，记录长按开始时间
           key_timer_context.long_press_start_time = HAL_GetTick();
           key_timer_context.long_press_counter = 1;
           
           // 生成短按事件
           KeyScanResult_TypeDef key_result = {
               .key_type = (Key_TypeDef)current_key_state,
               .event = KEY_EVENT_SHORT_PRESS,
               .is_pressed = true
           };
           
           KeyDriver_Process(&key_result);
       } else {
           // 按键保持按下状态，增加计数器
           key_timer_context.long_press_counter++;
           
           // 检查是否达到长按阈值（基于扫描次数）
           if (key_timer_context.long_press_counter >= KEY_SCAN_COUNTS_PER_LONG) {
               // 达到长按阈值，发送长按事件
               KeyScanResult_TypeDef key_result = {
                   .key_type = (Key_TypeDef)current_key_state,
                   .event = KEY_EVENT_LONG_PRESS,
                   .is_pressed = true
               };
               
               KeyDriver_Process(&key_result);
           }
       }
   } else if (prev_key_state != 0) {
       // 按键释放
       uint32_t press_duration = HAL_GetTick() - key_timer_context.long_press_start_time;
       
       if (press_duration >= KEY_LONG_PRESS_TIME_MS) {
           // 长按释放事件
           KeyScanResult_TypeDef key_result = {
               .key_type = (Key_TypeDef)prev_key_state,
               .event = KEY_EVENT_RELEASE,
               .is_pressed = false
           };
           
           KeyDriver_Process(&key_result);
       } else {
           // 短按释放事件
           KeyScanResult_TypeDef key_result = {
               .key_type = (Key_TypeDef)prev_key_state,
               .event = KEY_EVENT_RELEASE,
               .is_pressed = false
           };
           
           KeyDriver_Process(&key_result);
       }
       
       // 重置计数器
       key_timer_context.long_press_counter = 0;
   }
   
   // 更新last_key_state用于下次比较
   key_timer_context.last_key_state = prev_key_state;
}

// 定时器中断回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
   if (htim->Instance == TIM7) {
       // 1ms定时器中断，执行按键扫描
       KeyDriver_Timer_Scan();
   }
}

/* USER CODE END 0 */

/* Function to check if any key is pressed */
uint8_t KeyDriver_IsAnyKeyPressed(void)
{
   return (internal_key_state.state != KEY_INTERNAL_STATE_IDLE) ? true : false;
}

/* USER CODE END PFP */