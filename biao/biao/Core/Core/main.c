/* USER CODE BEGIN 2 */
// 应用层模块初始化
ZTJ_Init();                           // 测量状态机初始化 - 初始化测量模式和单位
LCDDisplay_Init_New();                // LCD显示初始化 - 初始化显示缓存和配置
USB_APP_Init();                       // USB应用初始化 - 初始化USB连接和通信
MX_SENSOR_SAMPLE_Init();              // 传感器采样初始化 - 初始化传感器GPIO和中断

// 设置传感器类型 - 根据实际硬件配置，这里暂时设为0.5um传感器
// 在实际应用中，这可能需要根据硬件检测或配置参数来确定
sensor_type = SENSOR_TYPE_0_5UM;      // 假设是0.5um传感器

// sys_tick = HAL_GetTick();             // 记录初始系统时间戳 - 注释掉未使用的变量赋值
/* USER CODE END 2 */