/**
 * @file    teaching.c
 * @brief   示教再现系统核心实现
 *
 * 设计要点:
 * 1. 示教阶段: 电机惰行, 仅记录编码器增量到RAM缓冲区,
 *    每攒满一个Flash页 (256字节 = 64个点) 就写入Flash一次.
 *    Flash写页耗时 <3ms, 发生在10ms控制节拍内.
 *
 * 2. 再现阶段: 每次从Flash预取一页 (64个点) 到 replay_buf[],
 *    按顺序取出目标速度, 通过独立的左右轮速度PID驱动电机.
 *
 * 3. Flash擦除: 在 Teaching_StartRecord() 中一次性擦除
 *    所有数据扇区 (约2秒), 通过UART提示用户等待.
 */

#include "teaching.h"
#include "motor.h"
#include "encoder.h"
#include "mpu6050.h"
#include "w25q64.h"
#include "pid.h"
#include "usart.h"
#include "tim.h"

#include <string.h>

/* Flash一页可存放多少个轨迹点 */
#define POINTS_PER_PAGE   (W25Q64_PAGE_SIZE / sizeof(TrajectoryPoint_t))  /* = 64 */

/* ---- 内部状态 ------------------------------------------ */

static SystemState_t sys_state    = SYS_IDLE;
static uint32_t      total_points = 0;   /* 录制/再现总点数 */
static uint32_t      current_idx  = 0;   /* 当前操作点的索引 */

/* 写缓冲区: 攒满一页后刷入Flash */
static uint8_t   write_buf[W25Q64_PAGE_SIZE];
static uint16_t  write_buf_cnt = 0;        /* 缓冲区内已有的点数 */
static uint32_t  flash_write_addr = TEACH_FLASH_DATA_ADDR;
static uint8_t   record_had_errors = 0;    /* 录制期间出现Flash写入失败 */

/* 读缓冲区: 每次从Flash读取一整页到此处 */
static TrajectoryPoint_t replay_buf[POINTS_PER_PAGE];
static uint32_t replay_buf_base = 0;       /* replay_buf[0] 对应的全局点索引 */

/* PID控制器 (左轮/右轮各一个) */
static PID_t pid_left;
static PID_t pid_right;

/* MPU6050数据 */
static MPU6050_Data_t mpu;

/* ---- 内部函数原型 -------------------------------------- */

static void FlushWriteBuf(void);
static void LoadReplayPage(uint32_t point_idx);

/* ---- 实现 ---------------------------------------------- */

void Teaching_Init(void)
{
    Motor_Init();
    Encoder_Init();

    /* #region agent log - 插桩 E: W25Q64前 (假设D: I2C/SPI挂起定位) */
    uart_printf("[DBG] E: before W25Q64_Init\r\n");
    /* #endregion */

    /* 初始化Flash */
    /* #region agent log - H-E/H-B: 检查W25Q64 JEDEC ID是否正确 */
    {
        uint8_t ok = W25Q64_Init();
        uart_printf("[DBG-HE] W25Q64_Init returned %u (%s)\r\n",
               (unsigned)ok, ok ? "OK" : "FAIL - SPI/wiring error!");
    }
    /* #endregion */

    /* #region agent log - 插桩 F: MPU6050前 */
    uart_printf("[DBG] F: before MPU6050_Init\r\n");
    /* #endregion */

    /* 初始化MPU6050 */
    if (!MPU6050_Init()) {
        uart_printf("[WARN] MPU6050 init failed. Check I2C1 wiring.\r\n");
    } else {
        uart_printf("[INFO] Calibrating gyro, keep still...\r\n");
        MPU6050_CalibrateGyro(&mpu, 100);
        uart_printf("[INFO] Gyro bias: %.2f LSB\r\n", (double)mpu.gz_bias);
    }

    /* #region agent log - 插桩 G: MPU6050初始化完成后 */
    uart_printf("[DBG] G: MPU6050 done\r\n");
    /* #endregion */

    /* PID初始参数 (Kp=5, Ki=0.3, Kd=0.1, 输出限幅=999) */
    PID_Init(&pid_left,  5.0f, 0.3f, 0.1f, (float)MOTOR_PWM_MAX);
    PID_Init(&pid_right, 5.0f, 0.3f, 0.1f, (float)MOTOR_PWM_MAX);

    sys_state = SYS_IDLE;

    uart_printf("[INFO] System ready. Press PA4=Record, PA5=Replay.\r\n");
}

/* -------------------------------------------------------- */

void Teaching_StartRecord(void)
{
    if (sys_state != SYS_IDLE) return;

    uart_printf("[INFO] Erasing Flash sectors 0~%u ...\r\n",
           (unsigned)(TEACH_DATA_SECTORS));

    /* 擦除头部扇区 */
    /* #region agent log - H-E/H-B3: 监控扇区0擦除并回读确认全0xFF */
    uart_printf("[DBG-HE] Erasing sector 0 (header)...\r\n");
    W25Q64_EraseSector(TEACH_FLASH_HEADER_ADDR);
    uart_printf("[DBG-HE] Sector 0 erase done.\r\n");
    {
        uint8_t era[4] = {0};
        W25Q64_Read(TEACH_FLASH_HEADER_ADDR, era, 4);
        uart_printf("[DBG-7E] POST-ERASE sec0[0..3]=0x%02X 0x%02X 0x%02X 0x%02X (expect FF FF FF FF)\r\n",
               era[0], era[1], era[2], era[3]);
    }
    /* #endregion */

    /* 擦除所有数据扇区 */
    for (uint32_t s = 1; s <= TEACH_DATA_SECTORS; s++) {
        /* #region agent log - H-E: 监控每个扇区擦除进度 */
        if (s == 1U) uart_printf("[DBG-HE] Erasing data sector 1...\r\n");
        /* #endregion */
        W25Q64_EraseSector(s * W25Q64_SECTOR_SIZE);
        /* #region agent log - H-E */
        if (s == 1U) uart_printf("[DBG-HE] Data sector 1 erase done.\r\n");
        /* #endregion */
        if (s % 8 == 0) {
            uart_printf("[INFO]   Erased %lu/%u sectors\r\n",
                   (unsigned long)s, (unsigned)TEACH_DATA_SECTORS);
        }
    }
    uart_printf("[INFO] Erase done. Starting recording...\r\n");

    /* 复位所有录制状态 */
    total_points      = 0;
    current_idx       = 0;
    write_buf_cnt     = 0;
    flash_write_addr  = TEACH_FLASH_DATA_ADDR;
    record_had_errors = 0;
    memset(write_buf, 0xFFU, sizeof(write_buf));

    /* 清除编码器基准 (丢弃上一次读数) */
    Encoder_GetLeftDelta();
    Encoder_GetRightDelta();
    Encoder_ResetTotal();

    /* 复位MPU角度 */
    MPU6050_ReadRaw(&mpu);
    MPU6050_ResetYaw(&mpu);

    /* 电机高阻: STBY=LOW -> TB6612所有输出High-Z, 用户可自由推动小车 */
    Motor_Enable(0);

    /* #region agent log - H1 post-fix: 验证修复后电机进入High-Z模式 */
    uart_printf("[DBG-H1] Motor_Enable(0): STBY=LOW -> TB6612 HIGH-Z (true freewheel). runId=post-fix\r\n");
    /* #endregion */

    sys_state = SYS_TEACHING;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* PC13 LED 亮: 录制中 */
    uart_printf("[REC] Recording started. Press PA4 again to stop.\r\n");
}

void Teaching_StopRecord(void)
{
    if (sys_state != SYS_TEACHING) return;

    sys_state = SYS_SAVING;

    /* 刷出缓冲区中剩余未满一页的数据 */
    if (write_buf_cnt > 0) {
        FlushWriteBuf();
    }

    /* 写Flash头部 */
    uint8_t header[16];
    uint32_t magic = TEACH_FLASH_MAGIC;
    uint32_t samplems = TEACH_SAMPLE_MS;
    memcpy(header + 0,  &magic,        4);
    memcpy(header + 4,  &total_points, 4);
    memcpy(header + 8,  &samplems,     4);
    memset(header + 12, 0xFFU,         4);  /* reserved */

    /* #region agent log - H-B1/B2: 打印写入前 header 原始字节，确认 CPU 侧数据正确 */
    uart_printf("[DBG-7E] PRE-WRITE header[0..3]=0x%02X 0x%02X 0x%02X 0x%02X\r\n",
           header[0], header[1], header[2], header[3]);
    /* #endregion */

    /* 写入并验证头部，最多重试3次（NOR Flash 间歇性写失败的可靠写入策略） */
    uint8_t header_write_ok = 0;
    {
        for (uint8_t retry = 0; retry < 3U; retry++) {
            W25Q64_WritePage(TEACH_FLASH_HEADER_ADDR, header, 16);

            /* #region agent log - H-B: 写后立刻回读验证 */
            uint8_t rb[4] = {0};
            W25Q64_Read(TEACH_FLASH_HEADER_ADDR, rb, 4);
            uart_printf("[DBG-7E] POST-WRITE(try%u) readback[0..3]=0x%02X 0x%02X 0x%02X 0x%02X\r\n",
                   (unsigned)retry, rb[0], rb[1], rb[2], rb[3]);
            /* #endregion */

            if (rb[0]==header[0] && rb[1]==header[1] &&
                rb[2]==header[2] && rb[3]==header[3]) {
                /* #region agent log */
                if (retry > 0) {
                    uart_printf("[DBG-7E] Header write OK after %u retries.\r\n", (unsigned)retry);
                }
                /* #endregion */
                header_write_ok = 1;
                break;
            }

            /* 写入失败：若还有下一次重试机会，才擦除 sector0 再试；
             * 最后一次失败时保留本次写入（比全0xFF更接近预期）。
             * 注意：原来无条件擦除的 Bug 会让最后一次的数据也丢失，
             * 导致 StartReplay 读到 magic=0xFFFFFFFF 而永远无法回放。 */
            if ((retry + 1U) < 3U) {
                uart_printf("[WARN] Header write mismatch (try %u), re-erasing sector0...\r\n",
                       (unsigned)(retry + 1U));
                W25Q64_EraseSector(TEACH_FLASH_HEADER_ADDR);
            } else {
                uart_printf("[WARN] Header write mismatch (try %u), keeping last write (no more retries).\r\n",
                       (unsigned)(retry + 1U));
            }
        }

        if (!header_write_ok) {
            uart_printf("[ERR] Header write failed after 3 retries! Flash may be damaged.\r\n");
            uart_printf("[ERR] PA5 replay disabled: header sector is invalid (often all 0xFF).\r\n");
        }
    }

    /* 录制结束后重新使能TB6612，再短路制动归零 */
    Motor_Enable(1);
    Motor_Stop();
    sys_state = SYS_IDLE;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* PC13 LED 灭: 录制结束 */

    /* #region agent log - H-C: 确认StopRecord正常返回IDLE，总点数不为0 */
    uart_printf("[DBG-7E] StopRecord done: sys_state=IDLE total_points=%lu\r\n",
           (unsigned long)total_points);
    /* #endregion */

    if (record_had_errors) {
        uart_printf("[WARN] Recording FINISHED WITH FLASH ERRORS! Replay may be unreliable.\r\n");
        uart_printf("[WARN] If replay behaves erratically, press PA4 to re-record.\r\n");
    }
    if (!header_write_ok) {
        uart_printf("[REC] Stopped (RAM had %lu samples) but Flash header NOT saved — motor replay will not start.\r\n",
               (unsigned long)total_points);
    } else {
        uart_printf("[REC] Stopped. %lu points saved (%.1f s).\r\n",
               (unsigned long)total_points,
               (double)(total_points * TEACH_SAMPLE_MS) / 1000.0);
    }
}

/* -------------------------------------------------------- */

void Teaching_StartReplay(void)
{
    /* #region agent log - H-C: 确认函数被调用且state==IDLE */
    uart_printf("[DBG-7E] StartReplay called, sys_state=%d\r\n", (int)sys_state);
    /* #endregion */
    if (sys_state != SYS_IDLE) {
        uart_printf("[DBG-7E] StartReplay EARLY RETURN: state != IDLE!\r\n");
        return;
    }

    /* 读取并验证Flash头部 */
    uint8_t header[16];
    W25Q64_Read(TEACH_FLASH_HEADER_ADDR, header, 16);

    uint32_t magic = 0, pts = 0;
    memcpy(&magic, header + 0, 4);
    memcpy(&pts,   header + 4, 4);

    /* #region agent log - H2: 打印Flash头部实际读到的值 */
    uart_printf("[DBG-H2] Flash header: magic=0x%08lX (expect 0x%08lX) pts=%lu\r\n",
           (unsigned long)magic, (unsigned long)TEACH_FLASH_MAGIC, (unsigned long)pts);
    /* #endregion */

    if (magic != TEACH_FLASH_MAGIC || pts == 0) {
        /* #region agent log - H-B/H-E: Flash校验失败，打印详细原因 */
        if (magic != TEACH_FLASH_MAGIC) {
            uart_printf("[DBG-7E] H-B CONFIRMED: magic mismatch! got=0x%08lX expect=0x%08lX\r\n",
                   (unsigned long)magic, (unsigned long)TEACH_FLASH_MAGIC);
        }
        if (pts == 0) {
            uart_printf("[DBG-7E] H-E CONFIRMED: pts==0 in Flash header!\r\n");
        }
        /* #endregion */
        uart_printf("[ERR] No valid trajectory in Flash. Record first!\r\n");
        return;
    }

    total_points = pts;
    current_idx  = 0;

    /* 确保TB6612已使能 (录制结束后STBY可能仍为LOW) */
    Motor_Enable(1);

    /* 预加载第一页数据 */
    LoadReplayPage(0);

    /* #region agent log - 假设B/D: 打印回放缓冲区前5个目标速度点 */
    uart_printf("[DBG-RPL] replay_buf pt[0..4]: (%d,%d) (%d,%d) (%d,%d) (%d,%d) (%d,%d)\r\n",
           (int)replay_buf[0].left_delta, (int)replay_buf[0].right_delta,
           (int)replay_buf[1].left_delta, (int)replay_buf[1].right_delta,
           (int)replay_buf[2].left_delta, (int)replay_buf[2].right_delta,
           (int)replay_buf[3].left_delta, (int)replay_buf[3].right_delta,
           (int)replay_buf[4].left_delta, (int)replay_buf[4].right_delta);
    /* #endregion */

    /* ---- 回放前数据完整性保护 ---- */
    /* 检查: 若录制期间有Flash写入错误, 仅警告而非阻断 */
    if (record_had_errors) {
        uart_printf("[WARN] Replay: recording had Flash write errors, trajectory may be imperfect.\r\n");
    }
    /* 安全检查: 若第一数据页全为0xFF(擦除态)则拒绝回放 — 此时PID会因tgt=-1积分失控 */
    if (replay_buf[0].left_delta  == (int16_t)0xFFFF &&
        replay_buf[0].right_delta == (int16_t)0xFFFF &&
        replay_buf[1].left_delta  == (int16_t)0xFFFF &&
        replay_buf[1].right_delta == (int16_t)0xFFFF) {
        uart_printf("[ERR] Replay BLOCKED: Flash page0 is fully erased (0xFF). Re-record first!\r\n");
        return;
    }

    /* 复位PID状态 */
    PID_Reset(&pid_left);
    PID_Reset(&pid_right);

    /* 清除编码器基准 */
    /* #region agent log - 假设C: 清零前后TIM2/TIM3计数器值 */
    uart_printf("[DBG-RPL] enc before_clear: TIM2=%u TIM3=%u\r\n",
           (unsigned)__HAL_TIM_GET_COUNTER(&htim2),
           (unsigned)__HAL_TIM_GET_COUNTER(&htim3));
    /* #endregion */
    Encoder_GetLeftDelta();
    Encoder_GetRightDelta();
    /* #region agent log - 假设C */
    uart_printf("[DBG-RPL] enc after_clear:  TIM2=%u TIM3=%u\r\n",
           (unsigned)__HAL_TIM_GET_COUNTER(&htim2),
           (unsigned)__HAL_TIM_GET_COUNTER(&htim3));

    /* #endregion */

    /* 复位MPU角度 */
    MPU6050_ReadRaw(&mpu);
    MPU6050_ResetYaw(&mpu);

    sys_state = SYS_REPLAYING;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* PC13 LED 亮: 回放中 */
    uart_printf("[RPL] Replay started. %lu points (%.1f s). Press PA5 to abort.\r\n",
           (unsigned long)total_points,
           (double)(total_points * TEACH_SAMPLE_MS) / 1000.0);
}

void Teaching_StopReplay(void)
{
    if (sys_state != SYS_REPLAYING) return;

    Motor_Brake();
    HAL_Delay(50);
    Motor_Stop();

    sys_state = SYS_IDLE;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* PC13 LED 灭: 回放中止 */
    uart_printf("[RPL] Stopped at point %lu / %lu.\r\n",
           (unsigned long)current_idx, (unsigned long)total_points);
}

/* -------------------------------------------------------- */

void Teaching_Task(void)
{
    /* 每10ms读取编码器增量 (必须每次都读, 否则计数器会溢出) */
    int16_t left_delta  = Encoder_GetLeftDelta();
    int16_t right_delta = Encoder_GetRightDelta();

    /* 更新MPU6050数据与偏航角 */
    MPU6050_ReadRaw(&mpu);
    MPU6050_UpdateYaw(&mpu, TEACH_SAMPLE_MS / 1000.0f);

    switch (sys_state) {

    /* ---- 示教录制 ---- */
    case SYS_TEACHING:
    {
        if (total_points >= TEACH_MAX_POINTS) {
            uart_printf("[WARN] Max recording length reached, auto-stop.\r\n");
            Teaching_StopRecord();
            break;
        }

        /* 将当前点追加到写缓冲区 */
        TrajectoryPoint_t pt;
        pt.left_delta  = left_delta;
        pt.right_delta = right_delta;

        /* #region agent log - H1/H-H: 每50点打印增量及TIM2/TIM3原始计数器 */
        if (total_points % 50U == 0U) {
            uart_printf("[DBG-H1] REC pt=%lu L_delta=%d R_delta=%d | TIM2=%u TIM3=%u\r\n",
                   (unsigned long)total_points, (int)left_delta, (int)right_delta,
                   (unsigned)__HAL_TIM_GET_COUNTER(&htim2),
                   (unsigned)__HAL_TIM_GET_COUNTER(&htim3));
        }
        /* #endregion */

        /* #region agent log - T2: 记录每10个点一次，以及所有差速>5的时刻，找转弯段 */
        {
            static int16_t _max_diff = 0;
            int16_t _diff = left_delta - right_delta;
            if (_diff < 0) _diff = -_diff;
            if (_diff > _max_diff) {
                _max_diff = _diff;
                uart_printf("[DBG-T2] NEW_MAX_DIFF pt=%lu L=%d R=%d diff=%d\r\n",
                       (unsigned long)total_points, (int)left_delta, (int)right_delta, (int)_diff);
            }
            if (total_points % 10U == 0U) {
                uart_printf("[DBG-T2] REC10 pt=%lu L=%d R=%d\r\n",
                       (unsigned long)total_points, (int)left_delta, (int)right_delta);
            } else if (_diff > 5) {
                uart_printf("[DBG-T2] TURN  pt=%lu L=%d R=%d diff=%d\r\n",
                       (unsigned long)total_points, (int)left_delta, (int)right_delta, (int)_diff);
            }
        }
        /* #endregion */

        memcpy(write_buf + write_buf_cnt * sizeof(TrajectoryPoint_t),
               &pt, sizeof(TrajectoryPoint_t));
        write_buf_cnt++;
        total_points++;

        /* 缓冲区满一页时写入Flash */
        if (write_buf_cnt >= POINTS_PER_PAGE) {
            FlushWriteBuf();
        }
        break;
    }

    /* ---- 轨迹再现 ---- */
    case SYS_REPLAYING:
    {
        if (current_idx >= total_points) {
            /* 全部播完, 平滑减速停止 */
            Motor_Brake();
            HAL_Delay(80);
            Motor_Stop();
            sys_state = SYS_IDLE;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* PC13 LED 灭: 回放完成 */
            uart_printf("[RPL] Replay complete! Total %lu points.\r\n",
                   (unsigned long)total_points);
            break;
        }

        /* 若当前点不在缓冲区内则重新加载一页 */
        if (current_idx < replay_buf_base ||
            current_idx >= replay_buf_base + POINTS_PER_PAGE) {
            LoadReplayPage(current_idx);
        }

        /* 取出目标速度 */
        uint32_t offset = current_idx - replay_buf_base;
        TrajectoryPoint_t *target = &replay_buf[offset];

        /* 速度PID: 目标=录制速度, 反馈=当前编码器增量 */
        float left_out  = PID_Calculate(&pid_left,
                                        (float)target->left_delta,
                                        (float)left_delta);
        float right_out = PID_Calculate(&pid_right,
                                        (float)target->right_delta,
                                        (float)right_delta);

        /* #region agent log - 假设A/B/C/E: 前20帧每帧打印，之后每50帧打印一次 */
        if (current_idx < 20U || current_idx % 50U == 0U) {
            uart_printf("[DBG-RPL] idx=%lu tgt_L=%d tgt_R=%d cur_L=%d cur_R=%d out_L=%d out_R=%d\r\n",
                   (unsigned long)current_idx,
                   (int)target->left_delta, (int)target->right_delta,
                   (int)left_delta, (int)right_delta,
                   (int)left_out, (int)right_out);
        }
        /* #endregion */

        /* #region agent log - T1: 打印PID积分值，检测右轮积分windup；前200帧+每50帧 */
        if (current_idx < 200U || current_idx % 50U == 0U) {
            uart_printf("[DBG-T1] idx=%lu intL=%.1f intR=%.1f outL=%d outR=%d tgtL=%d tgtR=%d\r\n",
                   (unsigned long)current_idx,
                   (double)pid_left.integral, (double)pid_right.integral,
                   (int)left_out, (int)right_out,
                   (int)target->left_delta, (int)target->right_delta);
        }
        /* #endregion */

        Motor_SetSpeed((int16_t)left_out, (int16_t)right_out);

        current_idx++;

        /* 每100点 (~1秒) 打印一次进度 */
        if (current_idx % 100U == 0U) {
            uart_printf("[RPL] %lu/%lu pts | L=%d R=%d | Yaw=%.1f\r\n",
                   (unsigned long)current_idx,
                   (unsigned long)total_points,
                   (int)left_delta, (int)right_delta,
                   (double)mpu.yaw);
        }
        break;
    }

    case SYS_IDLE:
    case SYS_SAVING:
    default:
        break;
    }
}

/* -------------------------------------------------------- */

SystemState_t Teaching_GetState(void)
{
    return sys_state;
}

uint32_t Teaching_GetPointCount(void)
{
    return total_points;
}

void Teaching_SetPID(float kp, float ki, float kd)
{
    PID_SetParams(&pid_left,  kp, ki, kd);
    PID_SetParams(&pid_right, kp, ki, kd);
    uart_printf("[PID] Kp=%.2f Ki=%.2f Kd=%.2f\r\n",
           (double)kp, (double)ki, (double)kd);
}

/* ---- 内部函数 ------------------------------------------ */

/**
 * 将写缓冲区内容刷入Flash (整页写入 + 写后回读验证)
 *
 * W25Q64 page write 存在偶发可靠性问题 (特别是页内 byte[2])，
 * 未验证的数据页腐败会在回放第0帧产生异常大的目标速度，
 * 导致电机爆发冲击 ("乱动").
 * 对策: 写后回读前4字节对比; 若失败且为扇区首页则擦后重试.
 */
static void FlushWriteBuf(void)
{
    if (write_buf_cnt == 0) return;

    /* 未满的部分填充0xFF (Flash擦除后默认值) */
    uint16_t used_bytes = (uint16_t)(write_buf_cnt * sizeof(TrajectoryPoint_t));
    if (used_bytes < W25Q64_PAGE_SIZE) {
        memset(write_buf + used_bytes, 0xFFU,
               (size_t)(W25Q64_PAGE_SIZE - used_bytes));
    }

    uint32_t write_addr  = flash_write_addr;
    uint32_t sector_base = write_addr & ~((uint32_t)(W25Q64_SECTOR_SIZE - 1U));
    uint8_t  write_ok    = 0;

    for (uint8_t retry = 0; retry < 3U; retry++) {
        W25Q64_WritePage(write_addr, write_buf, W25Q64_PAGE_SIZE);

        /* #region agent log - 修复验证: 回读前4字节确认写入正确 */
        uint8_t rb[4] = {0};
        W25Q64_Read(write_addr, rb, 4);
        /* #endregion */

        if (rb[0] == write_buf[0] && rb[1] == write_buf[1] &&
            rb[2] == write_buf[2] && rb[3] == write_buf[3]) {
            /* #region agent log */
            if (retry > 0) {
                uart_printf("[FIX] Data page @0x%06lX OK after %u retries. rb=%02X %02X %02X %02X\r\n",
                       (unsigned long)write_addr, (unsigned)retry,
                       rb[0], rb[1], rb[2], rb[3]);
            }
            /* #endregion */
            write_ok = 1;
            break;
        }

        /* #region agent log - 修复验证: 写入失败详情 */
        uart_printf("[WARN] Data page @0x%06lX verify fail(try%u): got=%02X %02X %02X %02X exp=%02X %02X %02X %02X\r\n",
               (unsigned long)write_addr, (unsigned)(retry + 1U),
               rb[0], rb[1], rb[2], rb[3],
               write_buf[0], write_buf[1], write_buf[2], write_buf[3]);
        /* #endregion */

        if (write_addr == sector_base && (retry + 1U) < 3U) {
            /* 扇区首页且还有后续重试机会: 擦除后重试 */
            W25Q64_EraseSector(sector_base);
        } else if (write_addr != sector_base) {
            /* 扇区中间页: 擦除会丢失已写数据, 不重试 */
            break;
        }
        /* 扇区首页最后一次(retry=2)失败: 不再擦除, 保留本次写入的数据
         * (比全0xFF的擦除状态更接近预期) */
    }

    if (!write_ok) {
        uart_printf("[ERR] Data page @0x%06lX write failed! Recording may be corrupted.\r\n",
               (unsigned long)write_addr);
        record_had_errors = 1;
    }

    flash_write_addr += W25Q64_PAGE_SIZE;
    write_buf_cnt     = 0;
    memset(write_buf, 0xFFU, sizeof(write_buf));
}

/**
 * 从Flash加载包含 point_idx 的那一页到 replay_buf
 */
static void LoadReplayPage(uint32_t point_idx)
{
    /* 按页对齐: 找到 point_idx 所在页的起始点索引 */
    replay_buf_base = (point_idx / POINTS_PER_PAGE) * POINTS_PER_PAGE;

    uint32_t flash_addr = TEACH_FLASH_DATA_ADDR
                        + replay_buf_base * sizeof(TrajectoryPoint_t);

    W25Q64_Read(flash_addr, (uint8_t *)replay_buf, sizeof(replay_buf));

    /* #region agent log - T1/T3: 打印每页前5点+所有异常点(|delta|>50)，检测Flash数据损坏 */
    uart_printf("[DBG-T3] PAGE base=%lu addr=0x%06lX pt[0..4]:(%d,%d)(%d,%d)(%d,%d)(%d,%d)(%d,%d)\r\n",
           (unsigned long)replay_buf_base, (unsigned long)flash_addr,
           (int)replay_buf[0].left_delta,  (int)replay_buf[0].right_delta,
           (int)replay_buf[1].left_delta,  (int)replay_buf[1].right_delta,
           (int)replay_buf[2].left_delta,  (int)replay_buf[2].right_delta,
           (int)replay_buf[3].left_delta,  (int)replay_buf[3].right_delta,
           (int)replay_buf[4].left_delta,  (int)replay_buf[4].right_delta);
    for (uint32_t _i = 0; _i < POINTS_PER_PAGE; _i++) {
        int16_t _l = replay_buf[_i].left_delta;
        int16_t _r = replay_buf[_i].right_delta;
        int16_t _al = (_l < 0) ? -_l : _l;
        int16_t _ar = (_r < 0) ? -_r : _r;
        if (_al > 50 || _ar > 50) {
            uart_printf("[DBG-T3] ANOMALY base=%lu off=%lu L=%d R=%d\r\n",
                   (unsigned long)replay_buf_base, (unsigned long)_i, (int)_l, (int)_r);
        }
    }
    /* #endregion */
}
