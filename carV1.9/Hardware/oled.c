#include "oled.h"
#include <stdio.h>
#include <string.h>

static uint8_t OLED_Buffer[OLED_HEIGHT / 8][OLED_WIDTH];

static const uint8_t font6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* 0 space */
    {0x00,0x00,0x5F,0x00,0x00,0x00}, /* 1 ! */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* 2 " */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* 3 # */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* 4 $ */
    {0x23,0x13,0x08,0x64,0x62,0x00}, /* 5 % */
    {0x36,0x49,0x55,0x22,0x50,0x00}, /* 6 & */
    {0x00,0x05,0x03,0x00,0x00,0x00}, /* 7 ' */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* 8 ( */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* 9 ) */
    {0x14,0x08,0x3E,0x08,0x14,0x00}, /* 10 * */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* 11 + */
    {0x00,0x50,0x30,0x00,0x00,0x00}, /* 12 , */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* 13 - */
    {0x00,0x60,0x60,0x00,0x00,0x00}, /* 14 . */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* 15 / */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 16 0 */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 17 1 */
    {0x42,0x61,0x51,0x49,0x46,0x00}, /* 18 2 */
    {0x21,0x41,0x45,0x4B,0x31,0x00}, /* 19 3 */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 20 4 */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 21 5 */
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, /* 22 6 */
    {0x01,0x71,0x09,0x05,0x03,0x00}, /* 23 7 */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 24 8 */
    {0x06,0x49,0x49,0x29,0x1E,0x00}, /* 25 9 */
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, /* 26 A */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* 27 B */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* 28 C */
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, /* 29 D */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* 30 E */
    {0x7F,0x09,0x09,0x09,0x01,0x00}, /* 31 F */
    {0x3E,0x41,0x49,0x49,0x7A,0x00}, /* 32 G */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* 33 H */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* 34 I */
    {0x20,0x40,0x41,0x3F,0x01,0x00}, /* 35 J */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* 36 K */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* 37 L */
    {0x7F,0x02,0x0C,0x02,0x7F,0x00}, /* 38 M */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* 39 N */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* 40 O */
    {0x7F,0x09,0x09,0x09,0x06,0x00}, /* 41 P */
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, /* 42 Q */
    {0x7F,0x09,0x19,0x29,0x46,0x00}, /* 43 R */
    {0x46,0x49,0x49,0x49,0x31,0x00}, /* 44 S */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* 45 T */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* 46 U */
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, /* 47 V */
    {0x7F,0x20,0x18,0x20,0x7F,0x00}, /* 48 W */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* 49 X */
    {0x07,0x08,0x70,0x08,0x07,0x00}, /* 50 Y */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* 51 Z */
};

static uint8_t OLED_IsAsciiPrintable(unsigned char c)
{
    return (c >= 32 && c <= 126);
}

static const uint8_t* OLED_GetFont(char chr)
{
    if (chr == ' ') return font6x8[0];
    if (chr == '!') return font6x8[1];
    if (chr == '"') return font6x8[2];
    if (chr == '#') return font6x8[3];
    if (chr == '$') return font6x8[4];
    if (chr == '%') return font6x8[5];
    if (chr == '&') return font6x8[6];
    if (chr == '\'') return font6x8[7];
    if (chr == '(') return font6x8[8];
    if (chr == ')') return font6x8[9];
    if (chr == '*') return font6x8[10];
    if (chr == '+') return font6x8[11];
    if (chr == ',') return font6x8[12];
    if (chr == '-') return font6x8[13];
    if (chr == '.') return font6x8[14];
    if (chr == '/') return font6x8[15];
    if (chr >= '0' && chr <= '9') return font6x8[16 + (chr - '0')];
    if (chr >= 'A' && chr <= 'Z') return font6x8[26 + (chr - 'A')];
    if (chr >= 'a' && chr <= 'z') return font6x8[26 + (chr - 'a')];
    return font6x8[0];
}

static void OLED_WriteCommand(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, data, 2, HAL_MAX_DELAY);
}

static void OLED_WriteData(const uint8_t *data, uint16_t len)
{
    uint8_t buf[17];
    buf[0] = 0x40;
    while (len > 0)
    {
        uint8_t send = len > 16 ? 16 : (uint8_t)len;
        memcpy(&buf[1], data, send);
        HAL_I2C_Master_Transmit(&hi2c1, OLED_I2C_ADDR, buf, send + 1, HAL_MAX_DELAY);
        data += send;
        len -= send;
    }
}

static void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_WriteCommand(0xB0 + y);
    OLED_WriteCommand(((x & 0xF0) >> 4) | 0x10);
    OLED_WriteCommand((x & 0x0F) | 0x00);
}

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    if (t) OLED_Buffer[y / 8][x] |= (1 << (y % 8));
    else   OLED_Buffer[y / 8][x] &= ~(1 << (y % 8));
}

void OLED_Refresh(void)
{
    for (uint8_t page = 0; page < OLED_HEIGHT / 8; page++)
    {
        OLED_SetPos(0, page);
        OLED_WriteData(OLED_Buffer[page], OLED_WIDTH);
    }
}

void OLED_Clear(void)
{
    memset(OLED_Buffer, 0x00, sizeof(OLED_Buffer));
    OLED_Refresh();
}

void OLED_DisplayOn(void)
{
    OLED_WriteCommand(0xAF);
}

void OLED_DisplayOff(void)
{
    OLED_WriteCommand(0xAE);
}

void OLED_Init(void)
{
    HAL_Delay(100);
    OLED_DisplayOff();
    OLED_WriteCommand(0x20); OLED_WriteCommand(0x02);
    OLED_WriteCommand(0xB0);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x10);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0x81); OLED_WriteCommand(0x7F);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0xA8); OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xA4);
    OLED_WriteCommand(0xD3); OLED_WriteCommand(0x00);
    OLED_WriteCommand(0xD5); OLED_WriteCommand(0xF0);
    OLED_WriteCommand(0xD9); OLED_WriteCommand(0x22);
    OLED_WriteCommand(0xDA); OLED_WriteCommand(0x12);
    OLED_WriteCommand(0xDB); OLED_WriteCommand(0x20);
    OLED_WriteCommand(0x8D); OLED_WriteCommand(0x14);
    OLED_Clear();
    OLED_DisplayOn();
}

void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size)
{
    (void)size;
    const uint8_t *font = OLED_GetFont(chr);
    for (uint8_t col = 0; col < 6; col++)
    {
        for (uint8_t row = 0; row < 8; row++)
        {
            OLED_DrawPoint(x + col, y + row, (font[col] >> row) & 0x01);
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    while (*str)
    {
        OLED_ShowChar(x, y, *str++, size);
        x += 6;
    }
    OLED_Refresh();
}

void OLED_ShowUTF8String(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    while (*str)
    {
        unsigned char c = (unsigned char)*str;
        if (OLED_IsAsciiPrintable(c))
        {
            OLED_ShowChar(x, y, (char)c, size);
            x += 6;
            str++;
        }
        else if ((c & 0x80) == 0x00)
        {
            str++;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            str += 2;
            x += 16;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            str += 3;
            x += 16;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            str += 4;
            x += 16;
        }
        else
        {
            str++;
        }
    }
    OLED_Refresh();
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%*lu", len, (unsigned long)num);
    OLED_ShowString(x, y, buf, size);
}

void OLED_ShowBitmap(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp)
{
    for (uint8_t row = 0; row < height; row++)
    {
        for (uint8_t col = 0; col < width; col++)
        {
            uint16_t index = col + (row / 8) * width;
            uint8_t bit = (bmp[index] >> (row % 8)) & 0x01;
            OLED_DrawPoint(x + col, y + row, bit);
        }
    }
    OLED_Refresh();
}

void OLED_ShowChinese(uint8_t x, uint8_t y, const uint8_t *bmp16x16)
{
    for (uint8_t row = 0; row < 16; row++)
    {
        uint8_t left = bmp16x16[row * 2];
        uint8_t right = bmp16x16[row * 2 + 1];
        for (uint8_t col = 0; col < 8; col++)
        {
            OLED_DrawPoint(x + col, y + row, (left >> (7 - col)) & 0x01);
        }
        for (uint8_t col = 0; col < 8; col++)
        {
            OLED_DrawPoint(x + 8 + col, y + row, (right >> (7 - col)) & 0x01);
        }
    }
    OLED_Refresh();
}
