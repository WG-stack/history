#ifndef __OLED_H
#define __OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "i2c.h"

#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDR (0x3C << 1)

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_DisplayOn(void);
void OLED_DisplayOff(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
void OLED_ShowUTF8String(uint8_t x, uint8_t y, const char *str, uint8_t size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowBitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp);
void OLED_ShowChinese(uint8_t x, uint8_t y, const uint8_t *bmp16x16);

#ifdef __cplusplus
}
#endif

#endif
