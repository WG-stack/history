/**
 * @file    w25q64.c
 * @brief   W25Q64 SPI NOR Flash 驱动实现
 */

#include "w25q64.h"
#include "spi.h"

/* SPI片选宏 */
#define CS_LOW()   HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET)

/* W25Q64 指令集 */
#define CMD_WRITE_ENABLE   0x06U  /* 写使能 */
#define CMD_WRITE_DISABLE  0x04U  /* 写禁止 */
#define CMD_READ_SR1       0x05U  /* 读状态寄存器1 */
#define CMD_READ_DATA      0x03U  /* 读数据 (最高25MHz) */
#define CMD_PAGE_PROGRAM   0x02U  /* 页编程 (最多256字节) */
#define CMD_SECTOR_ERASE   0x20U  /* 扇区擦除 4KB */
#define CMD_CHIP_ERASE     0xC7U  /* 全片擦除 */
#define CMD_JEDEC_ID       0x9FU  /* 读JEDEC ID */
#define CMD_RELEASE_PD     0xABU  /* 释放掉电模式 */

/* 状态寄存器1 bit0: BUSY */
#define SR1_BUSY_BIT       0x01U

/* -------------------------------------------------------- */

/**
 * 全双工 SPI：每发一字节必须同时收一字节并丢弃，否则 DR 堆积会 OVR，
 * 后续读 Flash 会错位（表现为页写入后回读某字节固定错误，与 debug.txt 一致）。
 */
static void SPI_Send(uint8_t byte)
{
    uint8_t rx_discard;
    HAL_SPI_TransmitReceive(&hspi2, &byte, &rx_discard, 1, 100);
}

static uint8_t SPI_Recv(void)
{
    uint8_t tx = 0xFFU;
    uint8_t rx = 0xFFU;
    HAL_SPI_TransmitReceive(&hspi2, &tx, &rx, 1, 100);
    return rx;
}

static void WriteEnable(void)
{
    CS_LOW();
    SPI_Send(CMD_WRITE_ENABLE);
    CS_HIGH();
}

uint8_t W25Q64_IsBusy(void)
{
    uint8_t sr;
    CS_LOW();
    SPI_Send(CMD_READ_SR1);
    sr = SPI_Recv();
    CS_HIGH();
    return (sr & SR1_BUSY_BIT) ? 1U : 0U;
}

void W25Q64_WaitDone(void)
{
    /* 轮询直到BUSY位清零 */
    /* #region agent log - H-E: 检测WaitDone是否死循环 (SPI不通则0xFF永远busy) */
    extern void uart_printf(const char *fmt, ...);
    uint32_t busy_cnt = 0;
    while (W25Q64_IsBusy()) {
        busy_cnt++;
        if (busy_cnt == 50000U) {
            uart_printf("[DBG-HE] WaitDone: still BUSY after 50000 polls! SPI may be dead.\r\n");
        }
        if (busy_cnt >= 200000U) {
            uart_printf("[DBG-HE] WaitDone: TIMEOUT after 200000 polls! Exiting to prevent hang.\r\n");
            break;  /* 插桩：强制退出防止无限死循环，便于观察后续行为 */
        }
    }
    if (busy_cnt > 0 && busy_cnt < 50000U) {
        uart_printf("[DBG-HE] WaitDone: done after %lu polls (normal).\r\n", (unsigned long)busy_cnt);
    }
    /* #endregion */
}

/* -------------------------------------------------------- */

uint8_t W25Q64_Init(void)
{
    CS_HIGH();
    HAL_Delay(5);

    /* 释放掉电模式 */
    CS_LOW();
    SPI_Send(CMD_RELEASE_PD);
    CS_HIGH();
    HAL_Delay(1);

    /* 验证JEDEC ID: Winbond W25Q64 = EF 40 17 */
    CS_LOW();
    SPI_Send(CMD_JEDEC_ID);
    uint8_t mfr  = SPI_Recv();
    uint8_t type = SPI_Recv();
    uint8_t cap  = SPI_Recv();
    CS_HIGH();

    return (mfr == 0xEFU && type == 0x40U && cap == 0x17U) ? 1U : 0U;
}

void W25Q64_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    CS_LOW();
    SPI_Send(CMD_READ_DATA);
    SPI_Send((uint8_t)((addr >> 16) & 0xFFU));
    SPI_Send((uint8_t)((addr >>  8) & 0xFFU));
    SPI_Send((uint8_t)( addr        & 0xFFU));
    /* 逐字节读取：直接调用 SPI_Recv()，避免依赖 HAL_SPI_Receive 在全双工主模式下
     * 内部调用 HAL_SPI_TransmitReceive(pData, pData, N) 时用 pData 同时充当 TX/RX
     * 缓冲区的隐式行为，使读取逻辑更清晰、可预测。 */
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = SPI_Recv();
    }
    CS_HIGH();
}

void W25Q64_WritePage(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (len == 0) return;
    if (len > W25Q64_PAGE_SIZE) len = W25Q64_PAGE_SIZE;

    WriteEnable();
    CS_LOW();
    SPI_Send(CMD_PAGE_PROGRAM);
    SPI_Send((uint8_t)((addr >> 16) & 0xFFU));
    SPI_Send((uint8_t)((addr >>  8) & 0xFFU));
    SPI_Send((uint8_t)( addr        & 0xFFU));
    /* 逐字节发送：每字节完整等待 TXE+RXNE+BSY，避免批量传输中的位错误 */
    for (uint16_t i = 0; i < len; i++) {
        SPI_Send(buf[i]);
    }
    CS_HIGH();

    /* 等待页编程完成 (典型0.7ms, 最大3ms) */
    W25Q64_WaitDone();
}

void W25Q64_EraseSector(uint32_t addr)
{
    /* 自动对齐到4KB边界 */
    addr &= ~(W25Q64_SECTOR_SIZE - 1U);

    WriteEnable();
    CS_LOW();
    SPI_Send(CMD_SECTOR_ERASE);
    SPI_Send((uint8_t)((addr >> 16) & 0xFFU));
    SPI_Send((uint8_t)((addr >>  8) & 0xFFU));
    SPI_Send((uint8_t)( addr        & 0xFFU));
    CS_HIGH();

    /* 等待扇区擦除完成 (典型60ms, 最大300ms) */
    W25Q64_WaitDone();
}

void W25Q64_EraseChip(void)
{
    WriteEnable();
    CS_LOW();
    SPI_Send(CMD_CHIP_ERASE);
    CS_HIGH();

    /* 等待全片擦除完成 (典型20s, 最大100s) */
    W25Q64_WaitDone();
}
