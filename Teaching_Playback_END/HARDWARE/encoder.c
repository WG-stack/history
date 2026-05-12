/**
 * @file    encoder.c
 * @brief   增量式光电编码器读取实现
 *
 * STM32F1 定时器编码器模式说明:
 *   TIM2/TIM3 工作在 TIM_ENCODERMODE_TI12 (双路4倍频)
 *   计数器为16位, 范围 0~65535, 溢出时自动翻转
 *   通过读取两次计数差值 (int16_t 强制转换) 得到有符号增量
 */

#include "encoder.h"
#include "tim.h"
#include "usart.h"

static int32_t left_total  = 0;
static int32_t right_total = 0;

static uint16_t left_last_cnt  = 0;
static uint16_t right_last_cnt = 0;

void Encoder_Init(void)
{
    /* #region agent log - H-TIM2A: 检查 HAL_TIM_Encoder_Start 返回值 */
    HAL_StatusTypeDef ret2 = HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    HAL_StatusTypeDef ret3 = HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
    uart_printf("[DBG-7E] TIM2_Start=%d TIM3_Start=%d (0=OK,1=ERR,2=BUSY,3=TIMEOUT)\r\n",
           (int)ret2, (int)ret3);
    /* #endregion */

    /* 记录初始计数值作为基准 */
    left_last_cnt  = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
    right_last_cnt = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);

    /* #region agent log - H-TIM2C fix verify: 读取 PA15/PB3 原始电平 (空闲时应为高电平=1) */
    {
        GPIO_PinState pa15 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15);
        GPIO_PinState pb3  = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3);
        GPIO_PinState pa6  = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
        GPIO_PinState pa7  = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
        uart_printf("[DBG-7E] GPIO idle: PA15=%d PB3=%d (TIM2 remap A/B) | PA6=%d PA7=%d (TIM3 A/B)\r\n",
               (int)pa15, (int)pb3, (int)pa6, (int)pa7);
    }
    /* #endregion */

    /* #region agent log - H-H/I post-fix: 上电后等500ms再读，验证TIM是否工作 */
    uart_printf("[DBG-HH] Encoder_Init: TIM2_CNT=%u TIM3_CNT=%u\r\n",
           (unsigned)left_last_cnt, (unsigned)right_last_cnt);
    HAL_Delay(500);
    uint16_t chk2 = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
    uint16_t chk3 = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    uart_printf("[DBG-HH] After 500ms idle: TIM2=%u TIM3=%u (non-zero=counting noise/signal)\r\n",
           (unsigned)chk2, (unsigned)chk3);
    /* #endregion */
}

int16_t Encoder_GetLeftDelta(void) 
{
    uint16_t cur = (uint16_t)__HAL_TIM_GET_COUNTER(&htim2);
    /*
     * 利用 int16_t 溢出特性自动处理计数器回绕:
     * 当计数器从65535溢出到0时, cur - last = 0 - 65535 = 1 (uint16 减法)
     * 强转为 int16_t 后正确得到 +1 (前进1脉冲)
     */
    int16_t delta  = (int16_t)(cur - left_last_cnt);

    /* #region agent log - H-TIM2C fix verify: 每200次打印TIM2计数器及PA15/PB3实时电平 */
    {
        static uint32_t _dbg_cnt = 0;
        _dbg_cnt++;
        if (_dbg_cnt % 200U == 0U) {
            GPIO_PinState pa15 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15);
            GPIO_PinState pb3  = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3);
            uart_printf("[DBG-7E] TIM2 cnt=%u delta=%d | PA15=%d PB3=%d\r\n",
                   (unsigned)cur, (int)delta, (int)pa15, (int)pb3);
        }
    }
    /* #endregion */

    left_last_cnt  = cur;
    left_total    += delta;
    return delta;
}

int16_t Encoder_GetRightDelta(void)
{
    uint16_t cur = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    int16_t delta  = (int16_t)(cur - right_last_cnt);
    right_last_cnt = cur;
    right_total   += delta;
    return delta;
}

int32_t Encoder_GetLeftTotal(void)
{
    return left_total;
}

int32_t Encoder_GetRightTotal(void)
{
    return right_total;
}

void Encoder_ResetTotal(void)
{
    left_total  = 0;
    right_total = 0;
}
