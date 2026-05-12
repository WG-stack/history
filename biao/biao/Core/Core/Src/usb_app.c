/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usb_app.c
  * @brief   USB应用层实现文件
  ******************************************************************************
  * @attention
  *
  * 该文件实现USB检测和处理功能
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usb_app.h"
#include "main.h"
#include "lcd_display.h"  // 用于显示USB状态 - 使用最终版LCD显示驱动
#include "key_driver.h"   // 可能需要与按键交互

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USB相关定义 */
#define USB_DETECT_PIN          LL_GPIO_PIN_11  // PA11 用于USB检测
#define USB_DETECT_PORT         GPIOA
#define USB_VBUS_PIN            LL_GPIO_PIN_12  // PA12 用于VBUS检测
#define USB_VBUS_PORT           GPIOA

/* USB检测相关宏 */
#define IS_USB_CONNECTED()      (LL_GPIO_IsInputPinSet(USB_DETECT_PORT, USB_DETECT_PIN) == 1)
#define IS_USB_POWERED()        (LL_GPIO_IsInputPinSet(USB_VBUS_PORT, USB_VBUS_PIN) == 1)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

static USB_StateTypeDef usb_state = USB_STATE_DISCONNECTED;
static USB_ModeTypeDef usb_mode = USB_MODE_CHARGING_ONLY;
static uint8_t usb_initialized = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  USB初始化
  * @param  None
  * @retval None
  */
void USB_APP_Init(void)
{
    // 配置USB检测引脚
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 使能GPIOA时钟
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    
    // 配置PA11和PA12为输入，用于检测USB连接状态
    GPIO_InitStruct.Pin = USB_DETECT_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;  // 下拉，USB插入时会被拉高
    LL_GPIO_Init(USB_DETECT_PORT, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = USB_VBUS_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;  // 下拉，VBUS存在时会被拉高
    LL_GPIO_Init(USB_VBUS_PORT, &GPIO_InitStruct);
    
    // 初始化USB状态
    usb_state = USB_STATE_DISCONNECTED;
    usb_mode = USB_MODE_CHARGING_ONLY;
    usb_initialized = 1;
    
    // 如果USB已经连接，则更新状态
    if(IS_USB_POWERED())
    {
        usb_state = USB_STATE_CONNECTED;
        USB_APP_ConnectCallback();
    }
}

/**
  * @brief  获取USB状态
  * @param  None
  * @retval USB状态
  */
USB_StateTypeDef USB_APP_GetState(void)
{
    if (!usb_initialized) {
        return USB_STATE_DISCONNECTED;
    }
    
    // 检测USB连接状态
    if(IS_USB_POWERED()) {
        if(usb_state == USB_STATE_DISCONNECTED) {
            // USB刚刚连接
            usb_state = USB_STATE_CONNECTED;
            USB_APP_ConnectCallback();
        }
    } else {
        if(usb_state != USB_STATE_DISCONNECTED) {
            // USB刚刚断开
            USB_APP_DisconnectCallback();
            usb_state = USB_STATE_DISCONNECTED;
        }
    }
    
    return usb_state;
}

/**
  * @brief  设置USB模式
  * @param  mode: USB模式
  * @retval None
  */
void USB_APP_SetMode(USB_ModeTypeDef mode)
{
    usb_mode = mode;
    
    // 根据模式更新LCD显示
    switch(mode)
    {
        case USB_MODE_CHARGING_ONLY:
            // 显示USB字符表示充电模式
            LCDDisplay_ShowString_New("USB");
            break;
        case USB_MODE_DATA_TRANSFER:
            // 显示USB字符表示数据传输模式
            LCDDisplay_ShowString_New("USB");
            break;
        case USB_MODE_DEBUG:
            // 显示USB字符表示调试模式
            LCDDisplay_ShowString_New("USB");
            break;
        default:
            break;
    }
}

/**
  * @brief  发送USB数据
  * @param  data: 数据指针
  * @param  length: 数据长度
  * @retval None
  */
void USB_APP_SendData(uint8_t* data, uint16_t length)
{
    // 实际的USB数据发送实现需要USB库的支持
    // 这里只是框架实现
    if(usb_state == USB_STATE_CONNECTED && 
       (usb_mode == USB_MODE_DATA_TRANSFER || usb_mode == USB_MODE_DEBUG))
    {
        // TODO: 实际的USB发送逻辑
        // USBD_CDC_SetTxBuffer(&hUsbDeviceFS, data, length);
        // USBD_CDC_TransmitPacket(&hUsbDeviceFS);
    }
}

/**
  * @brief  接收USB数据
  * @param  data: 数据指针
  * @param  length: 数据长度
  * @retval None
  */
void USB_APP_DataReceive(uint8_t* data, uint16_t length)
{
    // 实际的USB数据接收实现需要USB库的支持
    // 这里只是框架实现
    if(usb_state == USB_STATE_CONNECTED && 
       (usb_mode == USB_MODE_DATA_TRANSFER || usb_mode == USB_MODE_DEBUG))
    {
        // TODO: 实际的USB接收逻辑
        // USBD_CDC_SetRxBuffer(&hUsbDeviceFS, data);
        // USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    }
}

/**
  * @brief  USB电源管理
  * @param  None
  * @retval None
  */
void USB_APP_PowerManagement(void)
{
    // 检查USB电源状态
    if(IS_USB_POWERED()) {
        // USB供电，可以启用更多功能
        // 如果是电池供电模式，可以减少功耗
    } else {
        // 依赖电池供电，进入低功耗模式
    }
}

/**
  * @brief  USB连接回调
  * @param  None
  * @retval None
  */
void USB_APP_ConnectCallback(void)
{
    // USB连接时的处理
    LCDDisplay_ShowString_New("USB ");  // 显示USB连接状态
    HAL_Delay(1000);
    LCDDisplay_Clear_New();  // 清除LCD显示缓存，准备新的显示内容
    LCDDisplay_Update_New();  // 更新LCD显示，将清除操作应用到实际显示
    
    // 根据需要设置USB模式
    USB_APP_SetMode(USB_MODE_DATA_TRANSFER);
    
    // 可以在此处添加其他USB连接后的处理逻辑
}

/**
  * @brief  USB断开连接回调
  * @param  None
  * @retval None
  */
void USB_APP_DisconnectCallback(void)
{
    // USB断开连接时的处理
    LCDDisplay_ShowString_New("UOFF");  // 显示USB断开状态
    HAL_Delay(1000);
    LCDDisplay_Clear_New();  // 清除LCD显示缓存，准备新的显示内容
    LCDDisplay_Update_New();  // 更新LCD显示，将清除操作应用到实际显示
    
    // 恢复到非USB模式
    USB_APP_SetMode(USB_MODE_CHARGING_ONLY);
    
    // 可以在此处添加其他USB断开后的处理逻辑
}

/**
  * @brief  USB任务处理
  * @param  None
  * @retval None
  */
void USB_APP_Task(void)
{
    if (!usb_initialized) {
        return;
    }
    
    // 定期检查USB状态
    USB_APP_GetState();
    
    // 处理USB电源管理
    USB_APP_PowerManagement();
    
    // 根据USB状态决定是否需要进一步处理
    if(usb_state == USB_STATE_CONNECTED)
    {
        // 如果需要，可以在这里处理USB数据传输
        // 例如，响应主机请求或发送设备数据
    }
}

/* USER CODE END 0 */
