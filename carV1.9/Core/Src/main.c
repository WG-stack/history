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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "ultrasonic.h"
#include "buzzer.h"
#include "trace_follow.h"
#include "app_bluetooth_control.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint32_t g_distance_cm = 0;
static uint32_t g_ui_tick = 0;
static uint8_t g_buzzer_on = 0;
static uint16_t g_trace_left = 0;
static uint16_t g_trace_middle = 0;
static uint16_t g_trace_right = 0;
static float g_trace_error = 0.0f;
static uint8_t g_bt_rx_byte = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  Buzzer_Init();
  Ultrasonic_Init();
  OLED_Init();
  TraceFollow_Init();
  TraceFollow_SetSpeed(60);
  TraceFollow_SetPid(18.0f, 0.0f, 6.0f);
  TraceFollow_SetServoCenter(1500);
  TraceFollow_SetServoRange(1200, 1800);
  TraceFollow_Start();
  TraceFollow_ManualStop();
  AppBt_Init();
  AppBt_SetMode(APP_BT_MODE_MANUAL);
  AppBt_SetEchoDebug(1U);
  AppBt_SendText("BT:DEBUG ON\r\nMODE:MANUAL\r\n");
  if (HAL_UART_Receive_IT(&huart1, &g_bt_rx_byte, 1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_Delay(50);
  OLED_Clear();
  OLED_ShowString(0, 0, "CAR READY", 1);
  OLED_ShowString(0, 16, "AUTO MODE", 1);
  HAL_Delay(50);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint16_t trace_l = 0;
    uint16_t trace_m = 0;
    uint16_t trace_r = 0;
    float trace_error = 0.0f;
    char line[24];
    AppBtMode_t bt_mode = AppBt_GetMode();

    AppBt_Process();

    if (bt_mode == APP_BT_MODE_AUTO)
    {
      g_distance_cm = Ultrasonic_ReadDistanceCm();
      TraceFollow_HandleObstacle(g_distance_cm);
      TraceFollow_Update();
      TraceFollow_GetAdc(&trace_l, &trace_m, &trace_r);
      trace_error = TraceFollow_GetError();

      OLED_Clear();
      OLED_ShowString(0, 0, TraceFollow_IsObstacleActive() ? "OBSTACLE" : "AUTO MODE", 1);
      if (g_distance_cm == 0U)
      {
        snprintf(line, sizeof(line), "DIST: -- CM");
      }
      else
      {
        snprintf(line, sizeof(line), "DIST:%luCM", (unsigned long)g_distance_cm);
      }
      OLED_ShowString(0, 16, line, 1);

      snprintf(line, sizeof(line), "L%u M%u R%u", trace_l, trace_m, trace_r);
      OLED_ShowString(0, 32, line, 1);
      snprintf(line, sizeof(line), "E%.2f", trace_error);
      OLED_ShowString(0, 48, line, 1);

      if (++g_ui_tick >= 10U)
      {
        g_ui_tick = 0;
        Bluetooth_SendTraceState(trace_l, trace_m, trace_r, trace_error);
      }

      if (TraceFollow_IsObstacleActive())
      {
        if (g_buzzer_on == 0U)
        {
          Buzzer_On();
          g_buzzer_on = 1U;
        }
      }
      else
      {
        if (g_buzzer_on != 0U)
        {
          Buzzer_Off();
          g_buzzer_on = 0U;
        }
      }
    }
    else
    {
      OLED_Clear();
      OLED_ShowString(0, 0, "MANUAL MODE", 1);
      OLED_ShowString(0, 16, "BT CONTROL", 1);
      OLED_ShowString(0, 32, "F/B/L/R/S", 1);
      OLED_ShowString(0, 48, "A= AUTO", 1);

      if (g_buzzer_on != 0U)
      {
        Buzzer_Off();
        g_buzzer_on = 0U;
      }
    }

    HAL_Delay(5);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    AppBt_PushByte(g_bt_rx_byte);
    if (HAL_UART_Receive_IT(&huart1, &g_bt_rx_byte, 1) != HAL_OK)
    {
      Error_Handler();
    }

    switch (g_bt_rx_byte)
    {
      case 'M':
      case 'm':
        TraceFollow_ManualStop();
        AppBt_SetMode(APP_BT_MODE_MANUAL);
        AppBt_SendText("MODE:MANUAL\r\n");
        break;
      case 'A':
      case 'a':
        TraceFollow_ManualStop();
        AppBt_SetMode(APP_BT_MODE_AUTO);
        AppBt_SendText("MODE:AUTO\r\n");
        break;
      case 'F':
      case 'f':
        if (AppBt_GetMode() == APP_BT_MODE_MANUAL)
        {
          TraceFollow_ManualForward(300);
          AppBt_SendAck(APP_BT_CMD_FORWARD);
        }
        break;
      case 'B':
      case 'b':
        if (AppBt_GetMode() == APP_BT_MODE_MANUAL)
        {
          TraceFollow_ManualBackward(300);
          AppBt_SendAck(APP_BT_CMD_BACKWARD);
        }
        break;
      case 'L':
      case 'l':
        if (AppBt_GetMode() == APP_BT_MODE_MANUAL)
        {
          TraceFollow_ManualLeft(250);
          AppBt_SendAck(APP_BT_CMD_LEFT);
        }
        break;
      case 'R':
      case 'r':
        if (AppBt_GetMode() == APP_BT_MODE_MANUAL)
        {
          TraceFollow_ManualRight(250);
          AppBt_SendAck(APP_BT_CMD_RIGHT);
        }
        break;
      case 'S':
      case 's':
        TraceFollow_ManualStop();
        AppBt_SendAck(APP_BT_CMD_STOP);
        break;
      default:
        break;
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
