/**
 * @file    teaching.h
 * @brief   示教再现系统核心接口
 *
 * ─────────────────────────────────────────────
 * 工作模式说明
 * ─────────────────────────────────────────────
 *
 * 【示教模式 (Teaching)】
 *   - 用户手动推动小车沿期望轨迹运动
 *   - 电机处于惰行状态 (高阻, 轮子可自由转动)
 *   - 系统每10ms读取一次两路编码器增量并存入Flash
 *   - 按下PA4启动录制, 再次按下PA4停止录制
 *
 * 【再现模式 (Replay)】
 *   - 从Flash读取已录制的编码器速度序列
 *   - 以双环PID控制电机跟踪目标速度
 *   - 按下PA5启动再现, 再现结束或再次按PA5停止
 *
 * ─────────────────────────────────────────────
 * Flash存储布局 (W25Q64, 8MB)
 * ─────────────────────────────────────────────
 *   地址 0x000000 ~ 0x000FFF  扇区0: 头部信息 (16字节)
 *   地址 0x001000 ~ 0x01FFFF  扇区1~31: 轨迹点数据
 *
 *   头部结构 (16字节):
 *     magic[4]         : 0x54504259 ("TPBY")
 *     total_points[4]  : 录制的点数
 *     sample_ms[4]     : 采样周期 (ms), = TEACH_SAMPLE_MS
 *     reserved[4]      : 保留
 *
 *   轨迹点 (4字节/点):
 *     left_delta[2]    : 左轮编码器增量 (有符号, 脉冲/10ms)
 *     right_delta[2]   : 右轮编码器增量 (有符号, 脉冲/10ms)
 *
 *   最大录制容量: 31扇区 × 4096字节 / 4字节 = 31744点 ≈ 5.3分钟
 *
 * ─────────────────────────────────────────────
 * 系统状态机
 * ─────────────────────────────────────────────
 *   IDLE ──[PA4按下]──> TEACHING
 *   TEACHING ──[PA4按下]──> SAVING ──> IDLE
 *   IDLE ──[PA5按下]──> REPLAYING
 *   REPLAYING ──[PA5按下 或 数据播完]──> IDLE
 */

#ifndef __TEACHING_H__
#define __TEACHING_H__

#include "main.h"

/* ---- 可调参数 ------------------------------------------ */

/** 采样周期 (ms), 与主循环调用间隔一致 */
#define TEACH_SAMPLE_MS          10U

/** 录制数据扇区数 (不含头部扇区0), 每扇区约10秒 */
#define TEACH_DATA_SECTORS       31U

/** 最大录制点数 */
#define TEACH_MAX_POINTS         (TEACH_DATA_SECTORS * (4096U / 4U))

/**
 * Flash头部魔数
 * 小端存储: byte[0]=0x59, byte[1]=0x42, byte[2]=0xFF, byte[3]=0x54
 *
 * byte[2] 故意设为 0xFF：W25Q64 NOR Flash 擦除后默认全1，
 * byte[2]=0xFF 意味着该字节不需要任何 1→0 编程操作，
 * 从而规避 SPI 第7字节位置的可靠性问题（VCC 瞬降导致该位置
 * 编程电流不足时，其他需要大量翻转的字节会失败，0xFF 则不受影响）。
 */
#define TEACH_FLASH_MAGIC        0x54FF4259UL

/** Flash头部地址 (扇区0) */
#define TEACH_FLASH_HEADER_ADDR  0x000000UL

/** 轨迹数据起始地址 (扇区1) */
#define TEACH_FLASH_DATA_ADDR    0x001000UL

/* ---- 数据结构 ------------------------------------------ */

/** 单个轨迹采样点 (4字节) */
typedef struct {
    int16_t left_delta;   /* 左轮编码器增量 (脉冲/采样周期) */
    int16_t right_delta;  /* 右轮编码器增量 (脉冲/采样周期) */
} TrajectoryPoint_t;

/** 系统状态枚举 */
typedef enum {
    SYS_IDLE      = 0,  /* 空闲, 等待按键 */
    SYS_TEACHING  = 1,  /* 示教中, 手推录制 */
    SYS_SAVING    = 2,  /* 保存Flash (录制结束后短暂状态) */
    SYS_REPLAYING = 3,  /* 再现中, PID跟踪 */
} SystemState_t;

/* ---- 接口函数 ------------------------------------------ */

/**
 * @brief  系统初始化 (调用所有外设Init后调用此函数)
 * @note   含MPU6050陀螺仪零偏校准 (~1秒), 调用时小车须静止
 */
void Teaching_Init(void);

/**
 * @brief  10ms控制任务 (在主循环中每10ms调用一次)
 * @note   包含: 编码器读取、PID计算、数据存储/读取、MPU6050更新
 */
void Teaching_Task(void);

/**
 * @brief  获取当前系统状态
 */
SystemState_t Teaching_GetState(void);

/**
 * @brief  启动示教录制 (与按键处理程序配合使用)
 * @note   会先擦除Flash相关扇区 (~2秒), 期间UART输出进度
 */
void Teaching_StartRecord(void);

/**
 * @brief  停止示教录制, 保存头部信息
 */
void Teaching_StopRecord(void);

/**
 * @brief  启动轨迹再现
 * @note   若Flash中无有效数据, 本函数直接返回并在UART输出提示
 */
void Teaching_StartReplay(void);

/**
 * @brief  停止轨迹再现
 */
void Teaching_StopReplay(void);

/**
 * @brief  获取已录制/已再现的点数
 */
uint32_t Teaching_GetPointCount(void);

/**
 * @brief  动态设置PID参数 (可通过UART在线调参)
 */
void Teaching_SetPID(float kp, float ki, float kd);

#endif /* __TEACHING_H__ */
