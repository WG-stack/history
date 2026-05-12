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