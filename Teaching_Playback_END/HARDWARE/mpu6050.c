/**
 * @file    mpu6050.c
 * @brief   MPU6050 驱动实现
 */

#include "mpu6050.h"
#include "i2c.h"

/* 寄存器地址 */
#define REG_SMPLRT_DIV   0x19U  /* 采样率分频 */
#define REG_CONFIG       0x1AU  /* 配置 (DLPF) */
#define REG_GYRO_CONFIG  0x1BU  /* 陀螺仪量程 */
#define REG_ACCEL_CONFIG 0x1CU  /* 加速度计量程 */
#define REG_ACCEL_XOUT_H 0x3BU  /* 数据起始地址 (共14字节) */
#define REG_PWR_MGMT_1   0x6BU  /* 电源管理1 */
#define REG_WHO_AM_I     0x68U  /* 设备ID, 应为 0x68 */

/* 陀螺仪灵敏度: FS_SEL=1 -> ±500dps -> 65.5 LSB/dps */
#define GYRO_SCALE       65.5f

/* -------------------------------------------------------- */

static HAL_StatusTypeDef WriteReg(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(&hi2c1, MPU6050_I2C_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, &val, 1, 100);
}

static HAL_StatusTypeDef ReadRegs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c1, MPU6050_I2C_ADDR, reg,
                            I2C_MEMADD_SIZE_8BIT, buf, len, 100);
}

/* -------------------------------------------------------- */

uint8_t MPU6050_Init(void)
{
    uint8_t who = 0;

    /* 先延时等待MPU6050上电稳定 */
    HAL_Delay(100);

    /* 读取设备ID验证通信 */
    if (ReadRegs(REG_WHO_AM_I, &who, 1) != HAL_OK || who != 0x68U) {
        return 0;
    }

    /* 解除休眠, 选择内部8MHz振荡器作为时钟源 */
    WriteReg(REG_PWR_MGMT_1, 0x00U);
    HAL_Delay(10);

    /* 采样率分频: SMPLRT_DIV=9 -> 1kHz / (1+9) = 100Hz */
    WriteReg(REG_SMPLRT_DIV, 0x09U);

    /* DLPF配置: 低通滤波器带宽约94Hz (对10ms采样足够) */
    WriteReg(REG_CONFIG, 0x02U);

    /* 陀螺仪量程: FS_SEL=1 -> ±500dps, 灵敏度65.5 LSB/dps */
    WriteReg(REG_GYRO_CONFIG, 0x08U);

    /* 加速度计量程: AFS_SEL=0 -> ±2g, 灵敏度16384 LSB/g */
    WriteReg(REG_ACCEL_CONFIG, 0x00U);

    return 1;
}

void MPU6050_ReadRaw(MPU6050_Data_t *data)
{
    uint8_t buf[14];

    if (ReadRegs(REG_ACCEL_XOUT_H, buf, 14) != HAL_OK) {
        return;
    }

    data->ax = (int16_t)((buf[0]  << 8) | buf[1]);
    data->ay = (int16_t)((buf[2]  << 8) | buf[3]);
    data->az = (int16_t)((buf[4]  << 8) | buf[5]);
    /* buf[6..7] = 温度原始值 */
    int16_t raw_t = (int16_t)((buf[6]  << 8) | buf[7]);
    data->gx = (int16_t)((buf[8]  << 8) | buf[9]);
    data->gy = (int16_t)((buf[10] << 8) | buf[11]);
    data->gz = (int16_t)((buf[12] << 8) | buf[13]);

    /* 温度转换公式 (来自MPU6050数据手册) */
    data->temp = (float)raw_t / 340.0f + 36.53f;
}

void MPU6050_UpdateYaw(MPU6050_Data_t *data, float dt_s)
{
    /*
     * 去除零偏后积分陀螺仪Z轴角速度得到偏航角
     * 注意: 纯积分会漂移, 仅用于短时辅助, 不作主要反馈
     */
    float gz_dps = ((float)data->gz - data->gz_bias) / GYRO_SCALE;
    data->yaw += gz_dps * dt_s;
}

void MPU6050_CalibrateGyro(MPU6050_Data_t *data, uint16_t samples)
{
    float sum = 0.0f;
    MPU6050_Data_t tmp = {0};

    for (uint16_t i = 0; i < samples; i++) {
        MPU6050_ReadRaw(&tmp);
        sum += (float)tmp.gz;
        HAL_Delay(5);
    }

    data->gz_bias = sum / (float)samples;
}

void MPU6050_ResetYaw(MPU6050_Data_t *data)
{
    data->yaw = 0.0f;
}
