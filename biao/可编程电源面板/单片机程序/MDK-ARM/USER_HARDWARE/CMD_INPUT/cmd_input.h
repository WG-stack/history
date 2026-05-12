#ifndef _CMD_INPUT_H_
#define _CMD_INPUT_H_

#include "main.h"

/* 指令输入位定义 */
#define CMD_INPUT_1  0  // CMDin_1 (PB12)
#define CMD_INPUT_2  1  // CMDin_2 (PB13)
#define CMD_INPUT_3  2  // CMDin_3 (PB14)
#define CMD_INPUT_4  3  // CMDin_4 (PB15)

/* 指令输入状态结构体 */
typedef struct {
    uint8_t cmd1_state;      // 指令1当前状态 (0-低电平, 1-高电平)
    uint8_t cmd2_state;      // 指令2当前状态
    uint8_t cmd3_state;      // 指令3当前状态
    uint8_t cmd4_state;      // 指令4当前状态
    uint8_t cmd1_changed;    // 指令1状态变化标志
    uint8_t cmd2_changed;    // 指令2状态变化标志
    uint8_t cmd3_changed;    // 指令3状态变化标志
    uint8_t cmd4_changed;    // 指令4状态变化标志
} CMD_Input_State_t;

extern CMD_Input_State_t CMD_Input_State;

/**
 * @brief 动作指令输入初始化
 */
void CMD_Input_Init(void);

/**
 * @brief 扫描所有指令输入状态
 * @note 检测状态变化并更新标志
 */
void CMD_Input_Scan(void);

/**
 * @brief 读取指定指令输入状态
 * @param cmd_num 指令编号 (1-4)
 * @return 0-低电平, 1-高电平
 */
uint8_t CMD_Input_Read(uint8_t cmd_num);

/**
 * @brief 检查指定指令是否有状态变化
 * @param cmd_num 指令编号 (1-4)
 * @return 0-无变化, 1-有变化
 */
uint8_t CMD_Input_IsChanged(uint8_t cmd_num);

/**
 * @brief 清除指定指令的变化标志
 * @param cmd_num 指令编号 (1-4)
 */
void CMD_Input_ClearChanged(uint8_t cmd_num);

/**
 * @brief 清除所有指令的变化标志
 */
void CMD_Input_ClearAllChanged(void);

/**
 * @brief 获取所有指令输入状态(位图)
 * @return 4位状态值 (bit0-CMD1, bit1-CMD2, bit2-CMD3, bit3-CMD4)
 */
uint8_t CMD_Input_GetAll(void);

#endif
