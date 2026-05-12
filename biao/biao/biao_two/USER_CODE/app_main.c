/**
  ******************************************************************************
  * @file           : app_main.c
  * @brief          : 应用主程序实现
  ******************************************************************************
  */

#include "app_main.h"
#include <stdio.h>

typedef enum {
    SET_ITEM_MOVE_DIR = 0,
    SET_ITEM_AUTO_OFF,
    SET_ITEM_OUTPUT_MODE,
    SET_ITEM_DISPLAY_MULT,
    SET_ITEM_COUNT
} SettingItem_t;

/* Private variables ---------------------------------------------------------*/
static AppState_t app_state = STATE_POWER_OFF;
static ErrorCode_t error_code = ERR_NONE;
static uint32_t last_data_time = 0;
static SettingItem_t current_setting_item = SET_ITEM_MOVE_DIR;
static uint8_t has_key_msg = 0;
static KeyMessage_t last_key_msg;

/* Private function prototypes -----------------------------------------------*/
static void App_ProcessKey_Setting(KeyMessage_t *key);
static void App_SettingMenu_UpdateDisplay(void);

/**
  * @brief  应用初始化
  */
void App_Init(void)
{
    // 初始化各模块
    Settings_Init();
    Settings_Load();
    
    Power_Init();
    Key_Init();
    Measure_Init();
    LCD_Display_Init();
    Battery_Init();
    
    Sensor_GPIO_Init();
    USB_Output_Init();
    UART_BT_Init();
    
    // 临时：跳过自检，直接进入NORMAL状态测试SPI
    app_state = STATE_NORMAL;
    
    // 启动传感器电源和GPIO中断接收
    Power_SensorOn();
    Sensor_GPIO_EnableInterrupt();
    
    // 清屏，准备显示
    LCD_Display_Clear();
}

/**
  * @brief  应用主循环
  */
void App_Run(void)
{
    SensorData_t sensor_data;
    int i;
    static uint8_t test_mode = 1;

    has_key_msg = Key_GetMessage(&last_key_msg);
    if (has_key_msg) {
        if (last_key_msg.code == KEY_DOWN_ABS_SET &&
            last_key_msg.event == KEY_EVENT_LONG_PRESS) {
            app_state = STATE_SETTING_MENU;
            App_SettingMenu_UpdateDisplay();
            current_setting_item = SET_ITEM_MOVE_DIR;
        } else {
            //Power_WakeUp();//这一行注释掉暂时对整体无影响
        }
    }
    
    switch (app_state) {
        case STATE_POWER_OFF:
            // 待机模式，等待开机键
            // Power_EnterStopMode();  // 暂时关闭休眠功能用于测试
            break;
            
        case STATE_SELF_TEST:
            // 自检
            App_SelfTest();
            break;
            
        case STATE_NORMAL:
            // 正常测量模式 - SPI读取测试
            
            // 调试：显示一个固定数字，确认LCD工作正常
//            if (test_mode) {
//                // 显示"888888"测试LCD
//                for (i = 0; i < 6; i++) {
//                    LCD_Write_Digit(i, 8);
//                }
//                LCD_Display_Symbol(LCD_SYMBOL_UNIT_MM, 1);
//                test_mode = 0;  // 只显示一次
//            }
            
            // 测试：读取并显示SPI数据
            if (Sensor_GPIO_GetData(&sensor_data)) {
                last_data_time = HAL_GetTick();
                
                // 显示原始数据值（20位BCD，显示为6位数字）
//                raw = sensor_data.raw_data;
//                for (i = 5; i >= 0; i--) {
//                    LCD_Write_Digit(i, raw % 10);
//                    raw /= 10;
//                }
                
//                // 显示单位
                if (sensor_data.is_inch) {
                    LCD_Display_Symbol(LCD_SYMBOL_UNIT_INCH, 1);
                    LCD_Display_Symbol(LCD_SYMBOL_UNIT_MM, 0);
                } else {
                    LCD_Display_Symbol(LCD_SYMBOL_UNIT_MM, 1);
                    LCD_Display_Symbol(LCD_SYMBOL_UNIT_INCH, 0);
                }
                
                // 显示符号
//                LCD_Display_Symbol(LCD_SYMBOL_NEGATIVE, sensor_data.is_negative);
                
                // 显示小数点（假设2位小数）
//                LCD_Display_Dot(3, 1);  // 第4位后面显示小数点
//                
//                // 所有LCD写入完成后，统一刷新一次，避免中间状态鬼影
//                LCD_Display_Update();
				// 用测量模块处理 & 更新当前显示值
				Measure_Process(&sensor_data);
				// 根据测量数据统一更新 LCD
                App_UpdateDisplay();
            }
            
            // 重新使能传感器中断，允许下次采样
            Sensor_GPIO_EnableInterrupt();

            if (has_key_msg) {
                App_ProcessKey(&last_key_msg);
            }
            
            // 检查错误（已禁用，避免超时进入错误状态）
            // App_CheckError();
            
            // 更新自动关机
            Power_UpdateAutoOff();
            
            // 进入低功耗模式
            // Power_EnterStopMode();  // 暂时关闭休眠功能用于测试
            break;
            
        case STATE_SETTING_MENU:
            if (has_key_msg) {
                App_ProcessKey_Setting(&last_key_msg);
            }
            break;
            
        case STATE_ENGINEERING_MENU:
            // 工程设置菜单
            // TODO: 实现工程设置逻辑
            break;
            
        case STATE_TOL_SETTING:
            // 公差设置
            // TODO: 实现公差设置逻辑
            break;
            
        case STATE_ERROR:
            // 错误状态 - 显示错误代码
            LCD_Display_Clear();
            
            // 显示"Err"
            LCD_Write_Digit(0, 0xE);  // E
            LCD_Write_Digit(1, 0xF);  // r (显示为-)
            LCD_Write_Digit(2, 0xF);  // r (显示为-)
            
            // 显示错误代码数字
            switch (error_code) {
                case ERR_NO_DATA:
                    LCD_Write_Digit(3, 0);  // Err0
                    break;
                case ERR_NO_ORIGIN:
                    LCD_Write_Digit(3, 1);  // Err1
                    break;
                case ERR_DATA_UNSTABLE:
                    LCD_Write_Digit(3, 2);  // Err2
                    break;
                default:
                    LCD_Write_Digit(3, 0xF);  // Err-
                    break;
            }
            
            // 闪烁显示（500ms周期）
            HAL_Delay(500);
            break;
    }
}

/**
  * @brief  自检
  */
void App_SelfTest(void)
{
    // LCD全显2秒
    LCD_Display_Init();  // 当前已经点亮所有符号
    HAL_Delay(2000);
    
    // 清屏
    LCD_Display_Clear();
    
    // 启动传感器
    Power_SensorOn();
    Sensor_GPIO_EnableInterrupt();
    
    // 等待数据稳定
    uint32_t start_time = HAL_GetTick();
    SensorData_t sensor_data;
    uint8_t data_received = 0;
    
    while (HAL_GetTick() - start_time < 2000) {
        if (Sensor_GPIO_GetData(&sensor_data)) {
            data_received = 1;
            break;
        }
        HAL_Delay(10);
    }
    
    if (!data_received) {
        // Err0: 2秒无数据
        error_code = ERR_NO_DATA;
        app_state = STATE_ERROR;
        return;
    }
    
    // TODO: 检查原位信号和数据稳定性
    // 如果检测到原位信号且数据稳定，自动置零
    
    // 自检通过，进入正常模式
    app_state = STATE_NORMAL;
    last_data_time = HAL_GetTick();
}

/**
  * @brief  处理按键
  */
void App_ProcessKey(KeyMessage_t *key_msg)
{
    switch (key_msg->code) {
        case KEY_ON_ZERO:
            if (key_msg->event == KEY_EVENT_SHORT_PRESS) {
                // 清零
                Measure_Zero();
            } else if (key_msg->event == KEY_EVENT_LONG_PRESS) {
                // 关机
                Power_Off();
                app_state = STATE_POWER_OFF;
            }
            break;
            
        case KEY_DOWN_ABS_SET:
            if (key_msg->event == KEY_EVENT_SHORT_PRESS) {
                // 切换ABS/REL
                MeasureData_t *data = Measure_GetData();
                if (data->mode == MODE_ABS) {
                    Measure_SetMode(MODE_REL);
                } else {
                    Measure_SetMode(MODE_ABS);
                }
            } else if (key_msg->event == KEY_EVENT_LONG_PRESS) {
                // 长按在任意模式下由App_Run统一处理
            }
            break;
            
        case KEY_UP_IN_MM:
            if (key_msg->event == KEY_EVENT_LONG_PRESS) {
                // 切换公英制
                MeasureData_t *data = Measure_GetData();
                if (data->unit == UNIT_MM) {
                    Measure_SetUnit(UNIT_INCH);
                } else {
                    Measure_SetUnit(UNIT_MM);
                }
            }
            break;
            
        case KEY_TOL:
             if (key_msg->event == KEY_EVENT_LONG_PRESS) {
                 // 进入公差设置
                 app_state = STATE_TOL_SETTING;
             }
            break;
            
        case KEY_M:
            if (key_msg->event == KEY_EVENT_SHORT_PRESS) {
                // 切换分辨率
                MeasureData_t *data = Measure_GetData();
                Resolution_t res = data->resolution;
                res = (Resolution_t)((res + 1) % 3);
                Measure_SetResolution(res);
            } else if (key_msg->event == KEY_EVENT_LONG_PRESS) {
                // 切换MAX/MIN/TIR
                MeasureData_t *data = Measure_GetData();
                MeasureMode_t mode = data->mode;
                if (mode == MODE_ABS || mode == MODE_REL) {
                    Measure_SetMode(MODE_MAX);
                } else if (mode == MODE_MAX) {
                    Measure_SetMode(MODE_MIN);
                } else if (mode == MODE_MIN) {
                    Measure_SetMode(MODE_TIR);
                } else {
                    Measure_SetMode(MODE_ABS);
                }
            }
            break;
            
        default:
            break;
    }
}

/**
  * @brief  更新显示
  */
void App_UpdateDisplay(void)
{
    MeasureData_t *data = Measure_GetData();
    
    if (!data->data_updated) {
        return;
    }
    
    // 清除更新标志
    data->data_updated = 0;
    
    // 显示数值
    uint8_t decimal_places = 0;
/*     switch (data->resolution) {
        case RESOLUTION_0_0005MM:
            decimal_places = 4;
            break;
        case RESOLUTION_0_001MM:
            decimal_places = 3;
            break;
        case RESOLUTION_0_01MM:
            decimal_places = 2;
            break;
    } */
    if (data->unit == UNIT_MM) {
        // 公制下的小数位数
        switch (data->resolution) {
            case RESOLUTION_0_0005MM: decimal_places = 4; break;
            case RESOLUTION_0_001MM:  decimal_places = 3; break;
            case RESOLUTION_0_01MM:   decimal_places = 2; break;
        }
    } else {
        // 英制下数值变小，需要增加小数位数以保证精度
        switch (data->resolution) {
            case RESOLUTION_0_0005MM: decimal_places = 5; break; // 对应 0.00002"
            case RESOLUTION_0_001MM:  decimal_places = 5; break; // 对应 0.00005"
            case RESOLUTION_0_01MM:   decimal_places = 4; break; // 对应 0.0005"
        }
    }
    SystemSettings_t *settings = Settings_Get();
    float factor = 1.0f;
    switch (settings->display_multiplier) {
        case DISPLAY_NORMAL:
            factor = 1.0f;
            break;
        case DISPLAY_X2:
            factor = 2.0f;
            break;
        case DISPLAY_DIV2:
            factor = 0.5f;
            break;
        case DISPLAY_CUSTOM:
            factor = settings->custom_multiplier;
            break;
    }

    float display_value = data->current_value * factor;
    
    LCD_Display_Number(display_value, decimal_places);

    // 显示单位
    if (data->unit == UNIT_MM) {
        LCD_Display_Symbol(LCD_SYMBOL_UNIT_MM, 1);
        LCD_Display_Symbol(LCD_SYMBOL_UNIT_INCH, 0);
    } else {
        LCD_Display_Symbol(LCD_SYMBOL_UNIT_MM, 0);
        LCD_Display_Symbol(LCD_SYMBOL_UNIT_INCH, 1);
    }
    
    // 显示模式
    LCD_Display_Symbol(LCD_SYMBOL_MAX, data->mode == MODE_MAX);
    LCD_Display_Symbol(LCD_SYMBOL_MIN, data->mode == MODE_MIN);
    LCD_Display_Symbol(LCD_SYMBOL_TIR, data->mode == MODE_TIR);
    LCD_Display_Symbol(LCD_SYMBOL_REL, data->mode == MODE_REL);
    
    // 显示公差状态
    if (data->tol_enabled) {
        LCD_Display_Symbol(LCD_SYMBOL_TOL, 1);
        LCD_Display_Symbol(LCD_SYMBOL_OK, data->tol_status == TOL_OK);
    }
    
    // 显示指针
    // TODO: 根据pointer_value点亮对应的ps符号
    
    // 显示电池
    float battery_voltage = Power_GetBatteryVoltage();
    if (battery_voltage < 2.5f) {
        LCD_Display_Symbol(LCD_SYMBOL_BATTERY, 1);
    }
    
    LCD_Display_Update();

    switch (settings->output_mode) {
        case OUTPUT_USB:
            // USB_Output_SendMeasure(display_value, data->unit, data->resolution);
            break;
        case OUTPUT_WIRELESS:
            // UART_BT_SendMeasure(display_value, data->unit, data->resolution);
            break;
        case OUTPUT_NONE:
        default:
            break;
    }
}

/**
  * @brief  检查错误
  */
void App_CheckError(void)
{
    uint32_t current_time = HAL_GetTick();
    
    // 检查数据超时
    if (current_time - last_data_time > 2000) {
        error_code = ERR_NO_DATA;
        app_state = STATE_ERROR;
    }
}

static void App_ProcessKey_Setting(KeyMessage_t *key)
{
    SystemSettings_t *settings = Settings_Get();

    switch (key->code) {
        case KEY_UP_IN_MM:
            if (key->event == KEY_EVENT_SHORT_PRESS) {
                switch (current_setting_item) {
                    case SET_ITEM_MOVE_DIR:
                        if (settings->move_direction < DIR_DOWN_POSITIVE) {
                            settings->move_direction++;
                        }
                        break;
                    case SET_ITEM_AUTO_OFF:
                        if (settings->auto_off_time < AUTO_OFF_DISABLE) {
                            settings->auto_off_time++;
                        }
                        break;
                    case SET_ITEM_OUTPUT_MODE:
                        if (settings->output_mode < OUTPUT_NONE) {
                            settings->output_mode++;
                        }
                        break;
                    case SET_ITEM_DISPLAY_MULT:
                        if (settings->display_multiplier < DISPLAY_CUSTOM) {
                            settings->display_multiplier++;
                        }
                        break;
                    default:
                        break;
                }
                App_SettingMenu_UpdateDisplay();
            }
            break;

        case KEY_DOWN_ABS_SET:
            if (key->event == KEY_EVENT_SHORT_PRESS) {
                switch (current_setting_item) {
                    case SET_ITEM_MOVE_DIR:
                        if (settings->move_direction > DIR_UP_POSITIVE) {
                            settings->move_direction--;
                        }
                        break;
                    case SET_ITEM_AUTO_OFF:
                        if (settings->auto_off_time > AUTO_OFF_15MIN) {
                            settings->auto_off_time--;
                        }
                        break;
                    case SET_ITEM_OUTPUT_MODE:
                        if (settings->output_mode > OUTPUT_USB) {
                            settings->output_mode--;
                        }
                        break;
                    case SET_ITEM_DISPLAY_MULT:
                        if (settings->display_multiplier > DISPLAY_NORMAL) {
                            settings->display_multiplier--;
                        }
                        break;
                    default:
                        break;
                }
                App_SettingMenu_UpdateDisplay();
            }
            break;

        case KEY_M:
            if (key->event == KEY_EVENT_SHORT_PRESS) {
                current_setting_item = (SettingItem_t)((current_setting_item + 1) % SET_ITEM_COUNT);
                App_SettingMenu_UpdateDisplay();
            }
            break;

        case KEY_ON_ZERO:
            if (key->event == KEY_EVENT_LONG_PRESS) {
                Settings_Save();
                app_state = STATE_NORMAL;
            }
            break;

        default:
            break;
    }
}

static void App_SettingMenu_UpdateDisplay(void)
{
    SystemSettings_t *settings = Settings_Get();

    LCD_Display_Clear();

    switch (current_setting_item) {
        case SET_ITEM_MOVE_DIR:
            LCD_Write_Digit(0, 1); // 自定义段码：0 表示方向菜单
            LCD_Write_Digit(4, (settings->move_direction == DIR_UP_POSITIVE) ? 0 : 1);
            break;

        case SET_ITEM_AUTO_OFF:
            LCD_Write_Digit(0, 2); // 1 表示自动关机菜单
            LCD_Write_Digit(4, (uint8_t)settings->auto_off_time);
            break;

        case SET_ITEM_OUTPUT_MODE:
            LCD_Write_Digit(0, 3); // 2 表示输出菜单
            LCD_Write_Digit(4, (uint8_t)settings->output_mode);
            break;

        case SET_ITEM_DISPLAY_MULT:
            LCD_Write_Digit(0, 4); // 3 表示倍数菜单
            LCD_Write_Digit(4, (uint8_t)settings->display_multiplier);
            break;

        default:
            break;
    }

    LCD_Display_Update();
}
