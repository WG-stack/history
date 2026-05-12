#include "cmd_input.h"

CMD_Input_State_t CMD_Input_State = {0};  // 指令输入状态全局变量

static uint8_t last_cmd1_state = 0;  // 上次指令1状态
static uint8_t last_cmd2_state = 0;  // 上次指令2状态
static uint8_t last_cmd3_state = 0;  // 上次指令3状态
static uint8_t last_cmd4_state = 0;  // 上次指令4状态

/**
 * @brief 动作指令输入初始化
 * @note 4点无源输入控制，PB12-PB15
 */
void CMD_Input_Init(void)
{
    // GPIO已在CubeMX中配置为输入模式
    // 读取初始状态
    CMD_Input_Scan();
    
    // 清除变化标志
    CMD_Input_ClearAllChanged();
    
    // 保存初始状态
    last_cmd1_state = CMD_Input_State.cmd1_state;
    last_cmd2_state = CMD_Input_State.cmd2_state;
    last_cmd3_state = CMD_Input_State.cmd3_state;
    last_cmd4_state = CMD_Input_State.cmd4_state;
}

/**
 * @brief 扫描所有指令输入状态
 * @note 检测状态变化并更新标志，建议在定时器中周期调用
 */
void CMD_Input_Scan(void)
{
    uint8_t current_state;
    
    // 读取CMD1 (PB12)
    current_state = PBin(12);
    if(current_state != last_cmd1_state)
    {
        CMD_Input_State.cmd1_changed = 1;
        last_cmd1_state = current_state;
    }
    CMD_Input_State.cmd1_state = current_state;
    
    // 读取CMD2 (PB13)
    current_state = PBin(13);
    if(current_state != last_cmd2_state)
    {
        CMD_Input_State.cmd2_changed = 1;
        last_cmd2_state = current_state;
    }
    CMD_Input_State.cmd2_state = current_state;
    
    // 读取CMD3 (PB14)
    current_state = PBin(14);
    if(current_state != last_cmd3_state)
    {
        CMD_Input_State.cmd3_changed = 1;
        last_cmd3_state = current_state;
    }
    CMD_Input_State.cmd3_state = current_state;
    
    // 读取CMD4 (PB15)
    current_state = PBin(15);
    if(current_state != last_cmd4_state)
    {
        CMD_Input_State.cmd4_changed = 1;
        last_cmd4_state = current_state;
    }
    CMD_Input_State.cmd4_state = current_state;
}

/**
 * @brief 读取指定指令输入状态
 * @param cmd_num 指令编号 (1-4)
 * @return 0-低电平, 1-高电平
 */
uint8_t CMD_Input_Read(uint8_t cmd_num)
{
    switch(cmd_num)
    {
        case 1: return CMD_Input_State.cmd1_state;
        case 2: return CMD_Input_State.cmd2_state;
        case 3: return CMD_Input_State.cmd3_state;
        case 4: return CMD_Input_State.cmd4_state;
        default: return 0;
    }
}

/**
 * @brief 检查指定指令是否有状态变化
 * @param cmd_num 指令编号 (1-4)
 * @return 0-无变化, 1-有变化
 */
uint8_t CMD_Input_IsChanged(uint8_t cmd_num)
{
    switch(cmd_num)
    {
        case 1: return CMD_Input_State.cmd1_changed;
        case 2: return CMD_Input_State.cmd2_changed;
        case 3: return CMD_Input_State.cmd3_changed;
        case 4: return CMD_Input_State.cmd4_changed;
        default: return 0;
    }
}

/**
 * @brief 清除指定指令的变化标志
 * @param cmd_num 指令编号 (1-4)
 */
void CMD_Input_ClearChanged(uint8_t cmd_num)
{
    switch(cmd_num)
    {
        case 1: CMD_Input_State.cmd1_changed = 0; break;
        case 2: CMD_Input_State.cmd2_changed = 0; break;
        case 3: CMD_Input_State.cmd3_changed = 0; break;
        case 4: CMD_Input_State.cmd4_changed = 0; break;
        default: break;
    }
}

/**
 * @brief 清除所有指令的变化标志
 */
void CMD_Input_ClearAllChanged(void)
{
    CMD_Input_State.cmd1_changed = 0;
    CMD_Input_State.cmd2_changed = 0;
    CMD_Input_State.cmd3_changed = 0;
    CMD_Input_State.cmd4_changed = 0;
}

/**
 * @brief 获取所有指令输入状态(位图)
 * @return 4位状态值 (bit0-CMD1, bit1-CMD2, bit2-CMD3, bit3-CMD4)
 */
uint8_t CMD_Input_GetAll(void)
{
    uint8_t state = 0;
    
    if(CMD_Input_State.cmd1_state) state |= 0x01;
    if(CMD_Input_State.cmd2_state) state |= 0x02;
    if(CMD_Input_State.cmd3_state) state |= 0x04;
    if(CMD_Input_State.cmd4_state) state |= 0x08;
    
    return state;
}
