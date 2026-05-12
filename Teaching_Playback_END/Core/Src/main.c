/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "teaching.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/** 按键消抖时间 (ms) */
#define BTN_DEBOUNCE_MS   20U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/** 控制任务时间基准 */
static uint32_t last_ctrl_tick = 0;

/** 按键消抖状态 */
typedef struct {
    uint32_t press_time;   /* 检测到下降沿的时刻 */
    uint8_t  detecting;    /* 正在消抖中 */
    uint8_t  triggered;    /* 已触发 (单次有效) */
} BtnState_t;

static BtnState_t btn_luzhi  = {0};   /* PA4 录制键 */
static BtnState_t btn_huifang = {0};  /* PA5 回放键 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Button_Scan(void);
static uint8_t Button_IsTriggered(BtnState_t *btn, GPIO_TypeDef *port, uint16_t pin);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* #region agent log - LED闪烁辅助宏 (调试用) */
#define DBG_LED_PORT  GPIOC
#define DBG_LED_PIN   GPIO_PIN_13
/* LED低电平亮, blink n次 */
static void dbg_led_blink(uint8_t n) {
    for (uint8_t i = 0; i < n; i++) {
        HAL_GPIO_WritePin(DBG_LED_PORT, DBG_LED_PIN, GPIO_PIN_RESET); /* 亮 */
        HAL_Delay(150);
        HAL_GPIO_WritePin(DBG_LED_PORT, DBG_LED_PIN, GPIO_PIN_SET);   /* 灭 */
        HAL_Delay(150);
    }
    HAL_Delay(500); /* 组间间隔 */
}
/* #endregion */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* #region agent log - 插桩 A: GPIO初始化完成, 系统时钟正常 (假设C验证)
   * 现象: 上电后看到 1闪 -> 时钟+GPIO均正常, Error_Handler未触发
   * 若看不到任何闪烁 -> 假设C成立, 时钟挂起 */
  dbg_led_blink(1);   /* 1闪 = 到达此处 (时钟+所有外设Init完成) */
  /* #endregion */

  /* #region agent log - 插桩 B: UART原始发送测试 (假设A/B/C验证)
   * 现象: 串口收到 ">>UART_OK\r\n" -> UART硬件工作, 排除假设A/B/C
   * 若只看到LED 1闪但串口无数据 -> 假设A(接线)或B(波特率)成立 */
  {
      const uint8_t test_msg[] = ">>UART_OK\r\n";
      HAL_UART_Transmit(&huart1, (uint8_t*)test_msg, sizeof(test_msg)-1, 500);
  }
  /* #endregion */

  /* #region agent log - 插桩 C: Teaching_Init开始前 2闪 (假设D验证)
   * 现象: 看到 2闪 -> 代码能到达此处, Teaching_Init内部可能挂住 */
  dbg_led_blink(2);   /* 2闪 = 即将进入 Teaching_Init */
  /* #endregion */

  /* 所有外设初始化完成后, 启动系统 */
  Teaching_Init();

  /* #region agent log - 插桩 D: Teaching_Init完成后 3闪 (假设D验证)
   * 现象: 看到 3闪 -> Teaching_Init正常返回, 初始化全部完成
   * 若只看到 2闪卡住 -> Teaching_Init内部挂住 (假设D: I2C/SPI死锁) */
  dbg_led_blink(3);   /* 3闪 = Teaching_Init 正常完成 */
  /* #endregion */

  last_ctrl_tick = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /* ---- 按键扫描 (每次主循环执行, 非阻塞) ---- */
    Button_Scan();

    /* ---- 10ms 控制任务 ---- */
    uint32_t now = HAL_GetTick();
    if ((now - last_ctrl_tick) >= TEACH_SAMPLE_MS) {
        last_ctrl_tick = now;
        Teaching_Task();
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * 单个按键消抖检测 (下降沿触发, 按键低电平有效)
 * @param  btn   按键状态结构体
 * @param  port  GPIO端口
 * @param  pin   GPIO引脚
 * @return 1 = 本次扫描确认按下 (仅返回一次)
 */
static uint8_t Button_IsTriggered(BtnState_t *btn, GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t result = 0;
    GPIO_PinState cur_state = HAL_GPIO_ReadPin(port, pin);

    if (!btn->detecting) {
        if (cur_state == GPIO_PIN_SET) {
            /* 引脚回高 (松开): 解锁, 允许下一次按下触发 */
            btn->triggered = 0;
        } else if (!btn->triggered) {
            /* 检测到低电平, 且上次已完整松开: 开始消抖 */
            btn->detecting  = 1;
            btn->press_time = HAL_GetTick();
        }
        /* 若 triggered=1 且引脚仍低: 按键持续被按住, 忽略 */
    } else {
        /* 消抖窗口内持续检测 */
        if (cur_state == GPIO_PIN_SET) {
            /* 消抖期间松开: 视为抖动, 取消 */
            btn->detecting = 0;
        } else if ((HAL_GetTick() - btn->press_time) >= BTN_DEBOUNCE_MS) {
            /* 消抖通过, 触发一次 */
            btn->triggered = 1;
            btn->detecting = 0;
            result = 1;
        }
    }
    return result;
}

/**
 * 按键扫描与响应
 *   PA4 (Luzhi_Pin):   录制键
 *     空闲态   -> 开始录制
 *     录制态   -> 停止录制
 *   PA5 (Huifang_Pin): 回放键
 *     空闲态   -> 开始回放
 *     回放态   -> 停止回放
 */
static void Button_Scan(void)
{
    /* PA4: 录制键 */
    if (Button_IsTriggered(&btn_luzhi, Luzhi_GPIO_Port, Luzhi_Pin)) {
        SystemState_t st = Teaching_GetState();
        if (st == SYS_IDLE) {
            Teaching_StartRecord();
        } else if (st == SYS_TEACHING) {
            Teaching_StopRecord();
        }
    }

    /* #region agent log - H-D: 每500次主循环打印一次PA5原始电平（检测按键硬件） */
    {
        static uint32_t _pa5_dbg_cnt = 0;
        _pa5_dbg_cnt++;
        if (_pa5_dbg_cnt >= 5000U) {
            _pa5_dbg_cnt = 0;
            GPIO_PinState _pa5 = HAL_GPIO_ReadPin(Huifang_GPIO_Port, Huifang_Pin);
            uart_printf("[DBG-7E] PA5 raw=%d (0=LOW/pressed,1=HIGH/idle)\r\n", (int)_pa5);
        }
    }
    /* #endregion */
    /* PA5: 回放键 */
    if (Button_IsTriggered(&btn_huifang, Huifang_GPIO_Port, Huifang_Pin)) {
        SystemState_t st = Teaching_GetState();
        /* #region agent log - H-A/H-C/H-D: PA5触发时打印当前状态 */
        uart_printf("[DBG-7E] PA5 triggered! state=%d (0=IDLE,1=TEACH,2=SAVE,3=RPL)\r\n", (int)st);
        /* #endregion */
        if (st == SYS_IDLE) {
            Teaching_StartReplay();
        } else if (st == SYS_REPLAYING) {
            Teaching_StopReplay();
        } else {
            /* #region agent log - H-A: PA5在非IDLE状态被忽略 */
            uart_printf("[DBG-7E] PA5 IGNORED: state != IDLE and != REPLAYING!\r\n");
            /* #endregion */
        }
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
