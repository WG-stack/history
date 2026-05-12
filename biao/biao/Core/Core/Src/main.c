/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - 数字游标卡尺固件主程序
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * 本固件适用于数字游标卡尺项目，集成了LCD显示、按键控制、传感器采样、USB通信等功能
  * 主要特性包括：
  * - 高精度传感器数据采样（24位ADC数据读取）
  * - LCD显示驱动（支持多种测量单位和模式切换）
  * - 按键扫描与状态机处理
  * - USB数据传输功能
  * - 低功耗管理模式
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
// 系统和外设驱动头文件
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "lcd.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_rcc.h"
#include "stm32l1xx_hal_rcc_ex.h"  // 关键：包含 RCC 扩展宏定义
#include "stm32l1xx_hal_pwr.h"
#include "stm32l1xx_hal_pwr_ex.h" // 关键：包含 PWR 扩展函数声明
#include "key_driver.h"           // 解决按键相关隐式声明
//#include "simple_key_timer.h"     // 简化定时器按键驱动 - 已不再使用，注释掉
#include "lcd_display.h"          // 解决 LCDDisplay_Init 隐式声明
#include "usb_app.h"              // 解决 USB_APP_SendData 隐式声明
#include "debug_config.h"         // 调试配置头文件
#include "sensor_sample.h"
#include "data_processing.h"
#include "measurement_state_machine.h"
#include <stdlib.h>
#include "global_variables.h"
#include <math.h>  // 添加数学函数库，用于模拟传感器数据
#include <stdio.h> // 添加stdio.h用于sprintf函数
#include "stm32l1xx_hal_adc.h"    // 添加ADC驱动头文件

// 声明外部全局变量
extern volatile uint8_t sensor_data_ready_flag;

// 调试宏定义

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 主程序状态机枚举定义
typedef enum {
    MAIN_STATE_INIT = 0,        // 初始化状态
    MAIN_STATE_IDLE,            // 空闲状态
    MAIN_STATE_SENSOR_READ,     // 传感器读取状态
    MAIN_STATE_DATA_PROCESS,    // 数据处理状态
    MAIN_STATE_LCD_UPDATE,      // LCD更新状态
    MAIN_STATE_USB_COMM         // USB通信状态
} MainState_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 定义系统运行参数常量

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// 私有宏定义区域 - 用于定义仅在本文件中使用的宏

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
// 处理后的数据 - 由数据处理模块更新，用于LCD显示和USB传输
extern SensorData_TypeDef processed_data; // 改为外部声明 - 从全局变量中获取

// 主程序状态机变量
static MainState_t main_state = MAIN_STATE_INIT;  // 当前主程序状态


// 调试用模拟传感器数据变量
#ifdef DEBUG_SIMULATE_SENSOR_DATA
static uint32_t simulation_counter = 0;          // 模拟数据计数器
static int32_t simulated_sensor_value = 0;       // 模拟传感器值
#endif

// 添加TIM7中断计数器的外部声明（实际上应该在global_variables.c中定义）
// uint32_t tim7_interrupt_count = 0;  // 已移到global_variables.c中定义

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
// 系统级函数声明
void SystemClock_Config(void);            // 系统时钟配置函数

/* USER CODE BEGIN PFP */
// 私有函数原型声明 - 仅在本文件中使用的函数
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief 毫秒级延时函数
  * @param ms: 延时毫秒数
 * @retval None
  * @note 此函数使用HAL_GetTick()实现阻塞式延时，适用于短时间延时场景
  */
#ifdef ENABLE_DELAY_MS
static void Delay_ms(uint32_t ms)
{
    uint32_t start_tick = HAL_GetTick();
    while((HAL_GetTick() - start_tick) < ms);
}
#endif

// 函数已移除，因为未被使用

// 全局变量声明 - 从其他模块获取当前采样值
extern volatile Sample_sample_t current_sample;  // 当前传感器采样值
/* USER CODE END 0 */

/**
  * @brief  应用程序入口点 - 系统初始化和主循环
  * @retval int - 返回值（通常为0）
  * @note 这是程序的入口点，执行硬件初始化并进入无限主循环
  * 主循环包含按键扫描、传感器采样、数据处理、显示更新和数据传输等功能
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  /* 微控制器配置：初始化所有外设、Flash接口和SysTick */

  /* 重置所有外设，初始化Flash接口和SysTick */
  HAL_Init();  // HAL库初始化 - 配置NVIC、系统滴答定时器等

  /* USER CODE BEGIN Init */
  /* 用户定义的初始化代码 */

  /* USER CODE END Init */

  /* 配置系统时钟 */
  SystemClock_Config();  // 配置系统时钟源、频率和分频

  /* USER CODE BEGIN SysInit */
  /* 系统级初始化代码 */

  /* USER CODE END SysInit */

  /* 初始化所有配置的外设 */
  MX_GPIO_Init();        // GPIO初始化 - 配置所有GPIO引脚

  // 确保GPIO时钟已稳定
  HAL_Delay(1);
  
  MX_DMA_Init();         // DMA初始化 - 配置直接内存访问

  // 确保DMA时钟已稳定
  HAL_Delay(1);
  
  MX_ADC1_Init();        // ADC1初始化 - 配置模数转换器

  // 确保ADC初始化完成
  HAL_Delay(1);
  
  MX_LCD_Init();         // LCD初始化 - 配置液晶显示器

  // 确保LCD初始化完成
  HAL_Delay(1);
  
  MX_RTC_Init();         // RTC初始化 - 配置实时时钟

  // 确保RTC初始化完成
  HAL_Delay(1);
  
  MX_TIM2_Init();        // TIM2初始化 - 配置定时器2

  // 确保定时器初始化完成
  HAL_Delay(1);
  
  MX_TIM7_Init();        // TIM7初始化 - 配置定时器7 for key press timing

  // 确保定时器初始化完成
  HAL_Delay(1);

  // MX_TIM3_Init();     // TIM3初始化 - 可选定时器（已禁用）
  MX_USART1_UART_Init(); // USART1初始化 - 配置串口通信
   /* USER CODE BEGIN 2 */
   // 应用层模块初始化
   ZTJ_Init();                           // 测量状态机初始化 - 初始化测量模式和单位
   LCDDisplay_Init_New();                // LCD显示初始化 - 初始化显示缓存和配置
   USB_APP_Init();                       // USB应用初始化 - 初始化USB连接和通信
   MX_SENSOR_SAMPLE_Init();              // 传感器采样初始化 - 初始化传感器GPIO和中断
   KeyDriver_Init();                     // 按键驱动初始化 - 初始化按键扫描功能
  
   // 设置传感器类型 - 根据实际硬件配置，这里暂时设为0.5um传感器
   // 在实际应用中，这可能需要根据硬件检测或配置参数来确定
   sensor_type = SENSOR_TYPE_0_5UM;      // 假设是0.5um传感器
   
   // 启动按键扫描定时器 - 使用TIM7定时器进行按键扫描
   KeyDriver_Timer_Start();
   
   // sys_tick = HAL_GetTick();             // 记录初始系统时间戳 - 注释掉未使用的变量赋值
   /* USER CODE END 2 */

  /* 在主循环前增加所有lcd段全显的代码 - 用于测试LCD显示 */
  // 实现真正的全显：直接操作LCD RAM寄存器来点亮所有段
  // 需要获取LCD句柄以直接操作RAM寄存器
  // 先确保LCD已初始化
  LCDDisplay_Init_New();
  

  // 清除显示
  LCDDisplay_Clear_New();
  LCDDisplay_Update_New();
 uint32_t startLCD_time=0;
  /* USER CODE BEGIN WHILE */
  while (1)  // 无限主循环 - 系统运行的核心
  {
		  // 方法1: 使用LCD驱动层函数点亮所有段
  // 遍历所有COM和SEG组合，点亮所有段
		
		if (startLCD_time==0){
      for(int com = 0; com < 4; com++) {
      for(int seg = 0; seg <= 25; seg++) {
          // 使用内部函数点亮所有段 (如果可用)
          // 此处调用LCD驱动函数来设置RAM寄存器
          uint32_t ram_index = com * 2;  // 对应RAM寄存器索引
          uint32_t current_ram_value = hlcd.Instance->RAM[ram_index];
          uint32_t new_value = current_ram_value | (1UL << seg);
          HAL_LCD_Write(&hlcd, ram_index, current_ram_value, new_value);
				  HAL_LCD_UpdateDisplayRequest(&hlcd);
         }
      }
			HAL_Delay(5000);
		}
    
  // 发起显示更新请求

    // 不再使用main_loop_counter，改用TIM7 for timing

    // 恢复按键扫描 - 简化定时器按键扫描（由TIM7中断自动处理）
    // key_result = KeyDriver_Scan();        // 恢复原有按键扫描
    // KeyDriver_Process(&key_result);       // 恢复原有按键处理
    // 移除了主循环中的按键扫描，使用定时器中断方式处理按键
    // KeyDriver_Scan();                     // 注释掉额外按键扫描
    // KeyDriver_Scan();                     // 注释掉额外按键扫描
       // 检查中断调试标志 - 验证中断是否被触发
    if(interrupt_debug_flag) {
        // 中断已被触发，清除标志
        interrupt_debug_flag = false;
    }
    
    // 作为从机，只需在每次循环中读取全局传感器数据并处理
    // 数据处理 - 将原始传感器数据转换为显示格式
    // 使用原子操作避免与传感器采样中断冲突
    __disable_irq();  // 关闭中断以保护临界区
    uint8_t temp_sensor_flag = sensor_data_ready_flag;
    if(temp_sensor_flag) {
        sensor_data_ready_flag = false;  // 清除标志
    }
    __enable_irq();  // 重新开启中断
    
    if(temp_sensor_flag) {
        processed_data = DataProcessing_Process(current_sample.value); // 处理原始数据
    }
    
    // 检查测量状态是否发生变化，如果是，则更新processed_data中的状态信息
    // 这确保按键事件导致的状态变化（如单位切换、模式切换、分辨率切换）能正确反映到显示上
    static uint8_t last_unit = 255;      // 初始化为不可能的值
    static uint8_t last_mode = 255;      // 初始化为不可能的值
    static uint8_t last_resolution = 255; // 初始化为不可能的值，用于监测分辨率变化
    
    // 获取当前测量状态
    ZTJ_TypeDef* current_state = ZTJ_GetCurrentState();
    
    // 检查状态是否发生变化
    if(last_unit != current_state->unit ||
       last_mode != current_state->mode ||
       last_resolution != current_state->current_resolution) {
        // 状态发生变化，更新processed_data中的状态信息
        processed_data.unit = current_state->unit;
        processed_data.mode = current_state->mode;
        
        // 如果是分辨率发生变化，需要重新处理数据以应用新的分辨率设置
        if(last_resolution != current_state->current_resolution) {
            // 重新处理当前传感器数据以应用新的分辨率
            processed_data = DataProcessing_Process(current_sample.value);
        }
        
        // 更新记录的最后状态
        last_unit = current_state->unit;
        last_mode = current_state->mode;
        last_resolution = current_state->current_resolution;
    }
    
    // LCD显示更新 - 使用最新处理的数据更新显示
    // 添加显示更新节流，避免过度频繁的更新（每50ms最多更新一次）
    static uint32_t last_display_update = 0;
    uint32_t current_tick = HAL_GetTick();
    if ((current_tick - last_display_update) >= 50) {  // 50ms间隔更新一次显示
        LCDDisplay_ShowProcessedData(processed_data);  // 更新LCD显示
        last_display_update = current_tick;
    }
    // 设置EXTI中断优先级高于定时器中断，确保传感器采样优先
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);  // 最高优先级
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    
    // 当传感器数据准备好时，USB数据传输 - 将最新的处理数据显示给主机
  //  if(sensor_data_ready_flag)
  //  {
  //      USB_APP_SendData((uint8_t*)&processed_data, sizeof(SensorData_TypeDef));  // 通过USB发送数据
   //     sensor_data_ready_flag = 0;  // 清除标志
  //  }
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 用户代码扩展区域3
    // 主程序状态机实现
    switch(main_state)
    {
        // 初始化状态：系统启动时的初始化过程
        case MAIN_STATE_INIT:
            // 系统初始化完成后，切换到空闲状态
            main_state = MAIN_STATE_IDLE;
            // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 注释掉未使用的变量赋值
            break;
            
        // 空闲状态：等待各种事件触发
        case MAIN_STATE_IDLE:
            // 更新系统时间戳
            // sys_tick = HAL_GetTick();
            
            // 简化定时器按键扫描由TIM7中断自动处理
            // 不再需要主动扫描按键
            
            // 如果传感器数据就绪，进入传感器读取状态
            if(sensor_data_ready_flag)
            {
                main_state = MAIN_STATE_SENSOR_READ;  // 进入传感器读取状态
                // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            }
            else
            {
                // 保持在空闲状态
                HAL_Delay(1);  // 短暂延时，避免过度占用CPU
            }
            break;
            
        // 传感器读取状态：读取传感器数据
        case MAIN_STATE_SENSOR_READ:
            // 作为从机，只需读取全局传感器数据
            // 实际的传感器读取由中断服务程序完成，这里只是获取数据
            
            // 检查传感器数据是否就绪
            if(sensor_data_ready_flag)
            {
                main_state = MAIN_STATE_DATA_PROCESS;  // 进入数据处理状态
                // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            }
            else
            {
                // 如果数据未就绪，返回空闲状态
                main_state = MAIN_STATE_IDLE;
                // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            }
            break;
            
        // 数据处理状态：处理传感器数据
        case MAIN_STATE_DATA_PROCESS:
            // 数据处理 - 将原始传感器数据转换为显示格式
            if(sensor_data_ready_flag) {
                processed_data = DataProcessing_Process(current_sample.value); // 处理原始数据
                sensor_data_ready_flag = false;  // 清除标志
                
                main_state = MAIN_STATE_LCD_UPDATE;  // 进入LCD更新状态
                // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            }
            else
            {
                // 如果没有数据可处理，返回空闲状态
                main_state = MAIN_STATE_IDLE;
                // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            }
            break;
            
        // LCD更新状态：更新LCD显示
        case MAIN_STATE_LCD_UPDATE:
            // LCD显示更新 - 使用最新处理的数据更新显示
            LCDDisplay_ShowProcessedData(processed_data);  // 更新LCD显示
            
            // 显示更新完成后，进入USB通信状态或返回空闲状态
            main_state = MAIN_STATE_USB_COMM;
            // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            break;
            
        // USB通信状态：处理USB数据传输
        case MAIN_STATE_USB_COMM:
            // 当传感器数据准备好时，USB数据传输 - 将最新的处理数据显示给主机
            // if(sensor_data_ready_flag)
            // {
            //     USB_APP_SendData((uint8_t*)&processed_data, sizeof(SensorData_TypeDef));  // 通过USB发送数据
            //     sensor_data_ready_flag = 0;  // 清除标志
            // }
            
            // USB通信完成后，返回空闲状态
            main_state = MAIN_STATE_IDLE;
            // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            break;
            
        // 默认情况：未知状态，返回空闲状态
        default:
            main_state = MAIN_STATE_IDLE;
            // state_enter_time = HAL_GetTick();  // 记录进入状态的时间 - 已注释掉未使用的变量
            break;
    }
    // 不再使用main_loop_counter，改用TIM7 for timing
    // 减少延时，提高主循环响应速度，让显示更新更流畅
    HAL_Delay(200);  // 减少延时到5ms，提高响应速度
		 startLCD_time=1;
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}