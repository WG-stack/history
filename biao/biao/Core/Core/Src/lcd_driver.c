/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd_driver.c
  * @brief   This file provides code for the LCD driver module
  *          implementing segment LCD control for the displacement sensor project
  ******************************************************************************
  * @attention
  *
  * This module implements the LCD driver functions as specified in the project
  * requirements, including digit display, symbol control, and power management.
  *
  * 该模块提供了完整的LCD驱动功能，包括：
  * - 6位7段数码管显示控制
  * - 各种图标和符号显示
  * - 模拟指针显示
  * - LCD硬件资源管理
  * - 显示缓冲区管理
  *
  * COM/SEG映射关系：
  * - COM0 (物理COM1) 对应RAM0
  * - COM1 (物理COM2) 对应RAM2
  * - COM2 (物理COM3) 对应RAM4
  * - COM3 (物理COM4) 对应RAM6
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "lcd_driver.h"  // 这会间接包含data_processing.h

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/*
 * 7-segment digit patterns for common anode display (active low)
 * Each segment of a 7-segment display is controlled by a combination of COM and SEG signals
 * The 7-segment layout is as follows:
 *
 *    AAAA
 *   F    B
 *   F    B
 *    GGGG
 *   E    C
 *   E    C
 *    DDDD  DP
 *
 * Each segment (A, B, C, D, E, F, G) is activated by setting the corresponding bit to 1
 * Bit layout: dp,g,f,e,d,c,b,a (bit 7,6,5,4,3,2,1,0)
 *
 * NOTE: Each segment requires 4 COM lines for proper operation in multiplexed displays
 * This ensures each segment can be uniquely addressed via COM/SEG intersection
 */
static const uint8_t digit_patterns[10] = {
    0x3F,  // 0: abcdef (0b0011111)
    0x06,  // 1: bc     (0b0000110)
    0x5B,  // 2: abged  (0b01011011)
    0x4F,  // 3: abgcd (0b0100111)
    0x66,  // 4: fgbc   (0b0110)
    0x6D,  // 5: afgcd  (0b01101101)
    0x7D,  // 6: afebcd (0b01111101)
    0x07,  // 7: abc    (0b00000111)
    0x7F,  // 8: abcdefg(0b0111111)
    0x6F   // 9: abcdfg (0b01101111)
};

/* Digit segment mapping for each digit position */
static const lcd_digit_segments_t digit_seg_map[6] = {
    // Digit 1: Segments 1A, 1B, 1C, 1D, 1E, 1F, 1G, P1 (decimal point) - 8 segments total
    {LCD_SEG_1A, LCD_SEG_1B, LCD_SEG_1C, LCD_SEG_1D, LCD_SEG_1E, LCD_SEG_1F, LCD_SEG_1G, LCD_SEG_P1},
    // Digit 2: Segments 2A, 2B, 2C, 2D, 2E, 2F, 2G, P2 (decimal point) - 8 segments total
    {LCD_SEG_2A, LCD_SEG_2B, LCD_SEG_2C, LCD_SEG_2D, LCD_SEG_2E, LCD_SEG_2F, LCD_SEG_2G, LCD_SEG_P2},
    // Digit 3: Segments 3A, 3B, 3C, 3D, 3E, 3F, 3G, P3 (decimal point) - 8 segments total
    {LCD_SEG_3A, LCD_SEG_3B, LCD_SEG_3C, LCD_SEG_3D, LCD_SEG_3E, LCD_SEG_3F, LCD_SEG_3G, LCD_SEG_P3},
    // Digit 4: Segments 4A, 4B, 4C, 4D, 4E, 4F, 4G, P4 (decimal point) - 8 segments total
    {LCD_SEG_4A, LCD_SEG_4B, LCD_SEG_4C, LCD_SEG_4D, LCD_SEG_4E, LCD_SEG_4F, LCD_SEG_4G, LCD_SEG_P4},
    // Digit 5: Segments 5A, 5B, 5C, 5D, 5E, 5F, 5G, P5 (decimal point) - 8 segments total
    {LCD_SEG_5A, LCD_SEG_5B, LCD_SEG_5C, LCD_SEG_5D, LCD_SEG_5E, LCD_SEG_5F, LCD_SEG_5G, LCD_SEG_P5},
    // Digit 6: Segments 6A, 6B, 6C, 6D, 6E, 6F, 6G, SET (decimal point) - 8 segments total
    {LCD_SEG_6A, LCD_SEG_6B, LCD_SEG_6C, LCD_SEG_6D, LCD_SEG_6E, LCD_SEG_6F, LCD_SEG_6G, LCD_SEG_SET}
};

/* Array to map LCD segment enums to COM and SEG values */
static const lcd_com_seg_map_t lcd_segment_com_seg_map[LCD_SEG_MAX_ENUM + 1] = {
    [LCD_SEG_REV]      = {0, 0},   // COM1/SEG0
    [LCD_SEG_FWD]      = {1, 0},   // COM2/SEG0
    [LCD_SEG_SIGN]     = {2, 0},   // COM3/SEG0
    [LCD_SEG_POSITIVE] = {3, 0},   // COM4/SEG0
    
    [LCD_SEG_1F]       = {0, 1},   // COM1/SEG1
    [LCD_SEG_1G]       = {1, 1},   // COM2/SEG1
    [LCD_SEG_1E]       = {2, 1},   // COM3/SEG1
    [LCD_SEG_1D]       = {3, 1},   // COM4/SEG1
    
    [LCD_SEG_1A]       = {0, 2},   // COM1/SEG2
    [LCD_SEG_1B]       = {1, 2},   // COM2/SEG2
    [LCD_SEG_1C]       = {2, 2},   // COM3/SEG2
    [LCD_SEG_P1]       = {3, 2},   // COM4/SEG2
    
    [LCD_SEG_2F]       = {0, 3},   // COM1/SEG3
    [LCD_SEG_2G]       = {1, 3},   // COM2/SEG3
    [LCD_SEG_2E]       = {2, 3},   // COM3/SEG3
    [LCD_SEG_2D]       = {3, 3},   // COM4/SEG3
    
    [LCD_SEG_2A]       = {0, 4},   // COM1/SEG4
    [LCD_SEG_2B]       = {1, 4},   // COM2/SEG4
    [LCD_SEG_2C]       = {2, 4},   // COM3/SEG4
    [LCD_SEG_P2]       = {3, 4},   // COM4/SEG4
    
    [LCD_SEG_3F]       = {0, 5},   // COM1/SEG5
    [LCD_SEG_3G]       = {1, 5},   // COM2/SEG5
    [LCD_SEG_3E]       = {2, 5},   // COM3/SEG5
    [LCD_SEG_3D]       = {3, 5},   // COM4/SEG5
    
    [LCD_SEG_3A]       = {0, 6},   // COM1/SEG6
    [LCD_SEG_3B]       = {1, 6},   // COM2/SEG6
    [LCD_SEG_3C]       = {2, 6},   // COM3/SEG6
    [LCD_SEG_P3]       = {3, 6},   // COM4/SEG6
    
    [LCD_SEG_4F]       = {0, 7},   // COM1/SEG7
    [LCD_SEG_4G]       = {1, 7},   // COM2/SEG7
    [LCD_SEG_4E]       = {2, 7},   // COM3/SEG7
    [LCD_SEG_4D]       = {3, 7},   // COM4/SEG7
    
    [LCD_SEG_4A]       = {0, 8},   // COM1/SEG8
    [LCD_SEG_4B]       = {1, 8},   // COM2/SEG8
    [LCD_SEG_4C]       = {2, 8},   // COM3/SEG8
    [LCD_SEG_P4]       = {3, 8},   // COM4/SEG8
    
    [LCD_SEG_5F]       = {0, 9},   // COM1/SEG9
    [LCD_SEG_5G]       = {1, 9},   // COM2/SEG9
    [LCD_SEG_5E]       = {2, 9},   // COM3/SEG9
    [LCD_SEG_5D]       = {3, 9},   // COM4/SEG9
    
    [LCD_SEG_5A]       = {0, 10},  // COM1/SEG10
    [LCD_SEG_5B]       = {1, 10},  // COM2/SEG10
    [LCD_SEG_5C]       = {2, 10},  // COM3/SEG10
    [LCD_SEG_P5]       = {3, 10},  // COM4/SEG10
    
    [LCD_SEG_6F]       = {0, 11},  // COM1/SEG11
    [LCD_SEG_6G]       = {1, 11},  // COM2/SEG11
    [LCD_SEG_6E]       = {2, 11},  // COM3/SEG11
    [LCD_SEG_6D]       = {3, 11},  // COM4/SEG11
    
    [LCD_SEG_6A]       = {0, 12},  // COM1/SEG12
    [LCD_SEG_6B]       = {1, 12},  // COM2/SEG12
    [LCD_SEG_6C]       = {2, 12},  // COM3/SEG12
    [LCD_SEG_SET]      = {3, 12},  // COM4/SEG12
    
    [LCD_SEG_OK]       = {0, 13},  // COM1/SEG13
    [LCD_SEG_NG]       = {1, 13},  // COM2/SEG13
    [LCD_SEG_TOL]      = {2, 13},  // COM3/SEG13
    [LCD_SEG_REL]      = {3, 13},  // COM4/SEG13
    
    [LCD_SEG_MAX]      = {0, 14},  // COM1/SEG14
    [LCD_SEG_TIR]      = {1, 14},  // COM2/SEG14
    [LCD_SEG_MIN]      = {2, 14},  // COM3/SEG14
    [LCD_SEG_HOLD]     = {3, 14},  // COM4/SEG14
    
    [LCD_SEG_INS1]     = {0, 15},  // COM1/SEG15
    [LCD_SEG_INU1]     = {1, 15},  // COM2/SEG15
    [LCD_SEG_IN_P]     = {2, 15},  // COM3/SEG15
    [LCD_SEG_IN]       = {3, 15},  // COM4/SEG15
    
    [LCD_SEG_MM]       = {0, 16},  // COM1/SEG16
    [LCD_SEG_MM_P]     = {1, 16},  // COM2/SEG16
    [LCD_SEG_MMST]     = {2, 16},  // COM3/SEG16
    [LCD_SEG_MMUT]     = {3, 16},  // COM4/SEG16
    
    [LCD_SEG_LO_BATT] = {0, 17},  // COM1/SEG17
    [LCD_SEG_PS_PP]    = {1, 17},  // COM2/SEG17 (ps++)
    [LCD_SEG_PS_P15]   = {2, 17},  // COM3/SEG17 (ps+15)
    [LCD_SEG_PS_0]     = {3, 17},  // COM4/SEG17 (ps0)
    
    [LCD_SEG_PS_P14]   = {0, 18},  // COM1/SEG18 (ps+14)
    [LCD_SEG_PS_P13]   = {1, 18},  // COM2/SEG18 (ps+13)
    [LCD_SEG_PS_P12]   = {2, 18},  // COM3/SEG18 (ps+12)
    [LCD_SEG_PS_P11]   = {3, 18},  // COM4/SEG18 (ps+11)
    
    [LCD_SEG_PS_P10]   = {0, 19},  // COM1/SEG19 (ps+10)
    [LCD_SEG_PS_P9]    = {1, 19},  // COM2/SEG19 (ps+9)
    [LCD_SEG_PS_P8]    = {2, 19},  // COM3/SEG19 (ps+8)
    [LCD_SEG_PS_P7]    = {3, 19},  // COM4/SEG19 (ps+7)
    
    [LCD_SEG_PS_P6]    = {0, 20},  // COM1/SEG20 (ps+6)
    [LCD_SEG_PS_P5]    = {1, 20},  // COM2/SEG20 (ps+5)
    [LCD_SEG_PS_P4]    = {2, 20},  // COM3/SEG20 (ps+4)
    [LCD_SEG_PS_P3]    = {3, 20},  // COM4/SEG20 (ps+3)
    
    [LCD_SEG_PS_P2]    = {0, 21},  // COM1/SEG21 (ps+2)
    [LCD_SEG_PS_P1]    = {1, 21},  // COM2/SEG21 (ps+1)
    [LCD_SEG_PS_N1]    = {2, 21},  // COM3/SEG21 (ps-1)
    [LCD_SEG_PS_N2]    = {3, 21},  // COM4/SEG21 (ps-2)
    
    [LCD_SEG_PS_N3]    = {0, 22},  // COM1/SEG22 (ps-3)
    [LCD_SEG_PS_N4]    = {1, 22},  // COM2/SEG22 (ps-4)
    [LCD_SEG_PS_N5]    = {2, 22},  // COM3/SEG22 (ps-5)
    [LCD_SEG_PS_N6]    = {3, 22},  // COM4/SEG22 (ps-6)
    
    [LCD_SEG_PS_N7]    = {0, 23},  // COM1/SEG23 (ps-7)
    [LCD_SEG_PS_N8]    = {1, 23},  // COM2/SEG23 (ps-8)
    [LCD_SEG_PS_N9]    = {2, 23},  // COM3/SEG23 (ps-9)
    [LCD_SEG_PS_N10]   = {3, 23},  // COM4/SEG23 (ps-10)
    
    [LCD_SEG_PS_N11]   = {0, 24},  // COM1/SEG24 (ps-11)
    [LCD_SEG_PS_N12]   = {1, 24},  // COM2/SEG24 (ps-12)
    [LCD_SEG_PS_N13]   = {2, 24},  // COM3/SEG24 (ps-13)
    [LCD_SEG_PS_N14]   = {3, 24},  // COM4/SEG24 (ps-14)
    
    [LCD_SEG_PS_MM]    = {0, 25},  // COM1/SEG25 (ps--)
    [LCD_SEG_PS_N15]   = {1, 25},  // COM2/SEG25 (ps-15)
};

/* Unified function to get COM/SEG mapping for LCD segments */
uint8_t lcd_get_com_from_segment(lcd_segment_t segment, uint8_t *com, uint8_t *seg) {
    if(segment > LCD_SEG_MAX_ENUM) {
        return 0; // Invalid segment
    }
    
    *com = lcd_segment_com_seg_map[segment].com;
    *seg = lcd_segment_com_seg_map[segment].seg;
    
    return 1; // Success
}

/* USER CODE BEGIN 1 */

/* Timer for 30Hz update rate */
static uint32_t last_update_tick = 0;
#define LCD_UPDATE_INTERVAL_MS (1000 / 30)  // ~33ms for 30Hz

/* Function to read hardware LCD RAM register */
uint32_t lcd_read_hardware_ram(uint8_t ram_idx)
{
    // Directly read the hardware register value
    if(ram_idx <= 6 && (ram_idx % 2 == 0)) {  // Valid RAM registers 0, 2, 4, 6
        return hlcd.Instance->RAM[ram_idx];
    }
    return 0;
}

/* Function to update the entire display from buffer */
void lcd_refresh_display(void)
{
    // Request display update to show current content
    HAL_StatusTypeDef status = HAL_LCD_UpdateDisplayRequest(&hlcd);
    if(status != HAL_OK) {
        // Error handling if needed - check LCD controller status
        // Try to ensure UDR bit is set by attempting the request again
        HAL_LCD_UpdateDisplayRequest(&hlcd);
    }
    
    // Additional delay to ensure display update is processed
    HAL_Delay(1);
    
    last_update_tick = HAL_GetTick();  // Update timestamp
}

/* USER CODE END 1 */

/**
 * @brief Initialize the LCD driver module
 * @retval None
 *
 * 此函数初始化LCD驱动模块，执行以下操作：
 * 1. 初始化更新计时器
 * 2. 清除整个显示内容
 * 3. 强制更新显示以确保硬件被正确清除
 * 4. 延迟确保初始化完成
 */
void lcd_driver_init(void)
{
    // Initialize the LCD hardware (already done in MX_LCD_Init)
    last_update_tick = HAL_GetTick();  // Initialize update timer
    
    // Clear the display to start fresh
    lcd_clear_all();
    // Force update to ensure hardware is also cleared
    lcd_force_update_display();
    // Additional delay to ensure initialization completes
    HAL_Delay(10);
}

/**
 * @brief Clear the entire LCD display
 * @retval None
 */
void lcd_clear_all(void)
{
    // Clear all RAM registers directly in hardware
    for(int i = 0; i < 8; i++) {
        if(i == 0 || i == 2 || i == 4 || i == 6) {  // Valid RAM registers for LCD
            // Clear register in hardware - use mask to clear all bits in this register
            HAL_LCD_Write(&hlcd, i, 0xFFFFFFFF, 0x00000000);  // Clear register
        }
    }
    
    // Request immediate display update
    HAL_LCD_UpdateDisplayRequest(&hlcd);
}

/**
 * @brief  Update the LCD display (request update)
 * @retval None
 */
void lcd_update_display(void)
{
    // Check if enough time has passed for 30Hz update rate (33ms interval)
    uint32_t current_tick = HAL_GetTick();
    if ((current_tick - last_update_tick) < LCD_UPDATE_INTERVAL_MS) {
        // Not enough time has passed, skip update
        return;
    }
    
    // Request display update to show current content after all changes are written
    HAL_LCD_UpdateDisplayRequest(&hlcd);
    
    // Update the last update tick time
    last_update_tick = current_tick;
}

/**
  * @brief  Set a specific LCD segment
  * @param  com: COM line (0-3)
  * @param  seg: SEG line (0-25)
  * @retval None
  */
void lcd_set_segment(uint8_t com, uint8_t seg)
{
    lcd_set_ram_bit(com, seg, 1);
}

/**
  * @brief  Clear a specific LCD segment
  * @param  com: COM line (0-3)
  * @param  seg: SEG line (0-25)
  * @retval None
  */
void lcd_clear_segment(uint8_t com, uint8_t seg)
{
    lcd_set_ram_bit(com, seg, 0);
}

/**
  * @brief  Set or clear a specific bit in the LCD RAM
  * @param  com: COM line (0-3) where COM0 maps to physical COM1, COM1 to physical COM2, etc.
  * @param  seg: SEG line (0-25)
  * @param state: 1 to set, 0 to clear
 * @retval None
 *
 * 此函数用于直接操作LCD RAM寄存器中的特定位，实现对LCD显示内容的精确控制。
 * COM/SEG映射关系：
 * - COM0 (物理COM1) -> RAM0
 * - COM1 (物理COM2) -> RAM2
 * - COM2 (物理COM3) -> RAM4
 * - COM3 (物理COM4) -> RAM6
 *
 * 每个RAM寄存器包含32位，每位控制一个COM/SEG交叉点，从而控制一个LCD段。
 * 该函数不会立即更新显示，而是允许批量更新以提高效率。
 */
void lcd_set_ram_bit(uint8_t com, uint8_t seg, uint8_t state)
{
    // According to user info: COM1->RAM0, COM2->RAM2, COM3->RAM4, COM4->RAM6
    // This means com=0 (physical COM1) -> RAM0, com=1 (physical COM2) -> RAM2, etc.
    // Each RAM register contains bits for multiple SEG lines for a single COM
    // Each RAM register holds 32 bits, each bit controls one COM/SEG intersection
    // RAM0: controls COM0 for all SEG lines (SEG0-31)
    // RAM2: controls COM1 for all SEG lines (SEG0-31)
    // RAM4: controls COM2 for all SEG lines (SEG0-31)
    // RAM6: controls COM3 for all SEG lines (SEG0-31)
    uint8_t ram_idx = com * 2;        // COM0->RAM0, COM1->RAM2, COM2->RAM4, COM3->RAM6
    uint8_t bit_pos = seg;            // Each SEG corresponds to one bit position in the COM's RAM register
    
    // Make sure we don't exceed the available RAM registers (0, 2, 4, 6)
    if(ram_idx > 6) return;
    
    // Modify the specific bit based on the state using HAL function to ensure proper synchronization
    if(state) {
        HAL_LCD_Write(&hlcd, ram_idx, (1UL << bit_pos), (1UL << bit_pos));  // Set specific bit
    } else {
        HAL_LCD_Write(&hlcd, ram_idx, (1UL << bit_pos), 0);  // Clear specific bit
    }
    
    // Don't update display immediately for each bit - allow batch updates
    // The display will be updated at higher level when appropriate
}

/**
  * @brief  Write data to a specific LCD RAM register
  * @param ram_idx: RAM register index (0,2,4,6 for COM0-COM3 respectively)
  * @param data: 32-bit data to write
  * @retval None
  */
void lcd_write_ram(uint8_t ram_idx, uint32_t data)
{
    if(ram_idx <= 6 && (ram_idx % 2 == 0)) { // RAM registers 0, 2, 4, 6 are used for COM0-COM3
        // Read the current register value completely
        uint32_t current_value = hlcd.Instance->RAM[ram_idx];
        
        // Modify the entire register value (read-all modify-write approach)
        current_value = data;
        
        // Write the fully modified value back to the hardware register
        hlcd.Instance->RAM[ram_idx] = current_value;
        
        // Request immediate display update by calling HAL function
        HAL_LCD_UpdateDisplayRequest(&hlcd);
    }
}

/**
 * @brief Read data from a specific LCD RAM register
  * @param ram_idx: RAM register index (0,2,4,6 for COM0-COM3 respectively)
  * @retval 32-bit data from the register
  */
uint32_t lcd_read_ram(uint8_t ram_idx)
{
    if(ram_idx <= 6 && (ram_idx % 2 == 0)) {
        // Return data directly from hardware register
        return hlcd.Instance->RAM[ram_idx];
    }
    return 0;
}

/**
 * @brief Display a number on the LCD
 * @param  number: Number to display
 * @param  decimal_places: Number of decimal places to show
 * @retval None
 */
void lcd_display_number(float number, uint8_t decimal_places)
{
    // First, clear all digit positions to ensure no old content remains
    for(uint8_t i = 0; i < 6; i++) {
        const lcd_digit_segments_t *seg_map = &digit_seg_map[i];
        
        // Clear all segments for this digit (a, b, c, d, e, f, g)
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_a].com, lcd_segment_com_seg_map[seg_map->seg_a].seg, 0);  // a
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_b].com, lcd_segment_com_seg_map[seg_map->seg_b].seg, 0);  // b
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_c].com, lcd_segment_com_seg_map[seg_map->seg_c].seg, 0);  // c
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_d].com, lcd_segment_com_seg_map[seg_map->seg_d].seg, 0);  // d
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_e].com, lcd_segment_com_seg_map[seg_map->seg_e].seg, 0);  // e
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_f].com, lcd_segment_com_seg_map[seg_map->seg_f].seg, 0);  // f
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_g].com, lcd_segment_com_seg_map[seg_map->seg_g].seg, 0);  // g
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_dp].com, lcd_segment_com_seg_map[seg_map->seg_dp].seg, 0);  // dp
    }
    
    // Handle sign
    uint8_t negative = 0;
    if(number < 0) {
        negative = 1;
        number = -number; // Work with absolute value
    }
    
    // Show negative sign if needed
    if(negative) {
        lcd_show_plus_minus_symbols(-1); // Show minus sign
    } else {
        lcd_show_plus_minus_symbols(1); // Show plus sign or clear
    }
    
    // Calculate multiplier to extract the required number of digits
    int multiplier = 1;
    for(int i = 0; i < decimal_places; i++) {
        multiplier *= 10;
    }
    
    // Convert number to integer representation with required decimal places
    int temp_num = (int)(number * multiplier + 0.5); // Adding 0.5 for rounding
    
    // Prepare array to hold digits
    uint8_t digits[6] = {0}; // 6 digits to display
    uint8_t num_digits = 0; // Count of actual digits in the number
    
    // Use pointer to iterate through the digits array
    uint8_t *digits_ptr = digits;
    
    // Extract digits from right to left (least significant to most significant)
    for(int i = 5; i >= 0; i--) {
        if(temp_num > 0 || num_digits > 0 || i == decimal_places) {
            *(digits_ptr + i) = temp_num % 10;  // Using pointer arithmetic
            temp_num /= 10;
            num_digits++;
            
            // Break if we've extracted all significant digits and we're past the decimal place
            if(temp_num == 0 && i < 5 - decimal_places && num_digits > decimal_places) break;
        } else {
            *(digits_ptr + i) = 0; // Fill with 0
        }
    }
    
    // Determine the position of the first significant digit (first non-zero digit before decimal point)
    uint8_t first_sig_digit = 6; // Default to beyond the array size
    for(int i = 0; i < 6; i++) {
        if(i <= 5 - decimal_places) { // Only consider digits before decimal point
            if(*(digits_ptr + i) != 0) {
                first_sig_digit = i;
                break;
            }
        }
    }
    
    // Display each digit using pointer arithmetic, with decimal point if needed
    for(int i = 0; i < 6; i++) {
        uint8_t show_dp = 0;
        // Show decimal point before the decimal_places-th digit from the right
        if(decimal_places > 0 && i == 5 - decimal_places) {
            show_dp = 1;
        }
        
        // If this digit is before the first significant digit and it's a zero before the decimal point, clear it
        if(i < first_sig_digit && i <= 5 - decimal_places && *(digits_ptr + i) == 0) {
            // Digit is already cleared above, so just handle decimal point if needed
            const lcd_digit_segments_t *seg_map = &digit_seg_map[i];
            
            // Handle decimal point separately
            if(show_dp) {
                lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_dp].com, lcd_segment_com_seg_map[seg_map->seg_dp].seg, 1);
            } else {
                lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_dp].com, lcd_segment_com_seg_map[seg_map->seg_dp].seg, 0);
            }
        } else {
            lcd_display_digit_with_dp(i, *(digits_ptr + i), show_dp);  // Using pointer arithmetic with decimal point control
        }
    }
}

/**
 * @brief  Display a single digit at a specific position
 * @param  position: Position (0-5) for the 6-digit display
  * @param digit: Digit value (0-9)
  * @param show_dp: Show decimal point (1) or not (0)
  * @retval None
  */
void lcd_display_digit_with_dp(uint8_t position, uint8_t digit, uint8_t show_dp)
{
    if(position >= 6 || digit > 9) return;
    
    // Use pointer to access the digit segment mapping
    const lcd_digit_segments_t *seg_map = &digit_seg_map[position];
    uint8_t pattern = digit_patterns[digit];
    
    // Use the array mapping to get COM/SEG values directly
    const uint8_t seg_a = seg_map->seg_a;
    const uint8_t seg_b = seg_map->seg_b;
    const uint8_t seg_c = seg_map->seg_c;
    const uint8_t seg_d = seg_map->seg_d;
    const uint8_t seg_e = seg_map->seg_e;
    const uint8_t seg_f = seg_map->seg_f;
    const uint8_t seg_g = seg_map->seg_g;
    const uint8_t seg_dp = seg_map->seg_dp;  // Decimal point segment
    
    // Use the mapping array to get COM/SEG values directly
    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_a].com, lcd_segment_com_seg_map[seg_a].seg, (pattern >> 0) & 1);  // a
    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_b].com, lcd_segment_com_seg_map[seg_b].seg, (pattern >> 1) & 1);  // b
    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_c].com, lcd_segment_com_seg_map[seg_c].seg, (pattern >> 2) & 1);  // c
    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_d].com, lcd_segment_com_seg_map[seg_d].seg, (pattern >> 3) & 1);  // d
    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_e].com, lcd_segment_com_seg_map[seg_e].seg, (pattern >> 4) & 1);  // e
    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_f].com, lcd_segment_com_seg_map[seg_f].seg, (pattern >> 5) & 1);  // f
    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_g].com, lcd_segment_com_seg_map[seg_g].seg, (pattern >> 6) & 1);  // g
    
    // Set decimal point if required
    if(show_dp) {
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_dp].com, lcd_segment_com_seg_map[seg_dp].seg, 1);
    } else {
        lcd_set_ram_bit(lcd_segment_com_seg_map[seg_dp].com, lcd_segment_com_seg_map[seg_dp].seg, 0);
    }
    // Note: Display update is handled at higher level for efficiency
}

/**
  * @brief  Display a single digit at a specific position (backward compatibility)
 * @param  position: Position (0-5) for the 6-digit display
  * @param digit: Digit value (0-9)
  * @retval None
  */
void lcd_display_digit(uint8_t position, uint8_t digit)
{
    lcd_display_digit_with_dp(position, digit, 0);  // Don't show decimal point by default
}

/**
  * @brief  Display a character at a specific position (limited set)
  * @param  position: Digit position (0-4)
  * @param  ch: Character to display
  * @retval None
  */
void lcd_display_char(uint8_t position, char ch)
{
    if(position >= 6) return;
    
    uint8_t segments = 0;
    const lcd_digit_segments_t *seg_map = NULL;  // Declare variable at function beginning to avoid jump-around-initialization issue
    
    // Map characters to segment patterns (limited set)
    switch(ch) {
        case '0':
            segments = digit_patterns[0];
            break;
        case '1':
            segments = digit_patterns[1];
            break;
        case '2':
            segments = digit_patterns[2];
            break;
        case '3':
            segments = digit_patterns[3];
            break;
        case '4':
            segments = digit_patterns[4];
            break;
        case '5':
            segments = digit_patterns[5];
            break;
        case '6':
            segments = digit_patterns[6];
            break;
        case '7':
            segments = digit_patterns[7];
            break;
        case '8':
            segments = digit_patterns[8];
            break;
        case '9':
            segments = digit_patterns[9];
            break;
        case 'A':  // A: a,b,c,e,f,g
            segments = 0x77; // 0b01110111
            break;
        case 'B':  // B: a,b,c,d,e,f (similar to 8 but without top-right segment)
            segments = 0x7C; // 0b01111100
            break;
        case 'C':  // C: a,d,e,f
            segments = 0x39; // 0b00111001
            break;
        case 'D':  // D: a,b,c,d,e (like 8 but without top-left and bottom-right)
            segments = 0x5E; // 0b01011110
            break;
        case 'E':  // E: a,d,e,f,g
            segments = 0x79; // 0b01111001
            break;
        case 'F':  // F: a,e,f,g
            segments = 0x71; // 0b01110001
            break;
        case 'G':  // G: a,c,d,e,f (like C but with bottom segment)
            segments = 0x3D; // 0b00111101
            break;
        case 'H':  // H: b,c,e,f,g
            segments = 0x76; // 0b01110110
            break;
        case 'I':  // I: e,f (short vertical line)
            segments = 0x06; // 0b00000110
            break;
        case 'J':  // J: b,c,d
            segments = 0x0E; // 0b00001110
            break;
        case 'K':  // K: e,f,g (show as H since it's similar)
            segments = 0x76; // 0b01110110 (same as H)
            break;
        case 'L':  // L: d,e,f
            segments = 0x38; // 0b00111000
            break;
        case 'M':  // M: a,c,e (special short form)
            segments = 0x19; // 0b00011001 (a,c,e segments)
            break;
        case 'N':  // N: a,c,e,g (like H but with top-right and bottom-left)
            segments = 0x54; // 0b01010100
            break;
        case 'O':  // O: a,b,c,d,e,f (same as 0)
            segments = 0x3F; // 0b00111111
            break;
        case 'P':  // P: a,b,e,f,g
            segments = 0x73; // 0b01110011
            break;
        case 'Q':  // Q: a,b,c,d,f,g (same as 0 but with bottom-right)
            segments = 0x3F; // 0b00111111 (same as 0, differentiate by context)
            break;
        case 'R':  // R: a,b,e,f,g (like P but with bottom-right)
            segments = 0x77; // 0b01110111 (same as A but differentiate by context)
            break;
        case 'S':  // S: a,c,d,f,g (same as 5)
            segments = 0x6D; // 0b01101101
            break;
        case 'T':  // T: d,e,f,g
            segments = 0x78; // 0b01111000
            break;
        case 'U':  // U: b,c,d,e,f (like 0 but without top)
            segments = 0x3E; // 0b0011110
            break;
        case 'V':  // V: d,e (like upside-down A)
            segments = 0x30; // 0b00110000
            break;
        case 'W':  // W: b,c,d,f (like U but without middle crossbar)
            segments = 0x36; // 0b00110110 (b,c,d,f segments)
            break;
        case 'X':  // X: b,c,e,f (like + sign)
            segments = 0x36; // 0b00110110 (same as W)
            break;
        case 'Y':  // Y: b,d,f,g
            segments = 0x6E; // 0b01101110
            break;
        case 'Z':  // Z: a,b,d,e (diagonal pattern)
            segments = 0x5B; // 0b01011011
            break;
        case '-':  // Dash: g only
            segments = 0x40; // 0b1000000 (bit 6 for g segment)
            break;
        case '_':  // Underscore: d only
            segments = 0x08; // 0b00001000
            break;
        case '.':  // Decimal point only
            // Just show decimal point, no segments for the digit itself
            // Specifically target only the decimal point for this position, ensuring other segments are off
            seg_map = &digit_seg_map[position];
            // Clear all segments for this digit first (a, b, c, d, e, f, g)
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_a].com, lcd_segment_com_seg_map[seg_map->seg_a].seg, 0);  // a
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_b].com, lcd_segment_com_seg_map[seg_map->seg_b].seg, 0);  // b
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_c].com, lcd_segment_com_seg_map[seg_map->seg_c].seg, 0);  // c
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_d].com, lcd_segment_com_seg_map[seg_map->seg_d].seg, 0);  // d
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_e].com, lcd_segment_com_seg_map[seg_map->seg_e].seg, 0);  // e
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_f].com, lcd_segment_com_seg_map[seg_map->seg_f].seg, 0);  // f
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_g].com, lcd_segment_com_seg_map[seg_map->seg_g].seg, 0);  // g
            // Then set only the decimal point
            lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_dp].com,
                           lcd_segment_com_seg_map[seg_map->seg_dp].seg, 1);
            return; // Return early to avoid calling lcd_display_char_segments again
        case ' ':  // Space: none
        default:
            segments = 0x00;
            break;
    }
    
    lcd_display_char_segments(position, segments);
}

/**
  * @brief  Display character segments at a position
  * @param  position: Digit position (0-5)
  * @param  segments: Segment pattern (bits 0-6 for a-g segments)
  * @param  show_dp: Show decimal point (1) or not (0)
  * @retval None
  */
void lcd_display_char_segments_with_dp(uint8_t position, uint8_t segments, uint8_t show_dp)
{
    if(position >= 6) return;
    
    // Use pointer arithmetic to access digit segment mapping
    const lcd_digit_segments_t * const seg_map = digit_seg_map + position;
    
    // Use pointer to access the segment-to-COM/SEG mapping
    const uint8_t seg_a = seg_map->seg_a;
    const uint8_t seg_b = seg_map->seg_b;
    const uint8_t seg_c = seg_map->seg_c;
    const uint8_t seg_d = seg_map->seg_d;
    const uint8_t seg_e = seg_map->seg_e;
    const uint8_t seg_f = seg_map->seg_f;
    const uint8_t seg_g = seg_map->seg_g;
    const uint8_t seg_dp = seg_map->seg_dp;  // Decimal point segment
    
    // Use pointer arithmetic to access the COM/SEG mapping
    const lcd_com_seg_map_t * const com_seg_map = lcd_segment_com_seg_map;
    
    lcd_set_ram_bit((com_seg_map + seg_a)->com, (com_seg_map + seg_a)->seg, (segments >> 0) & 1);  // a
    lcd_set_ram_bit((com_seg_map + seg_b)->com, (com_seg_map + seg_b)->seg, (segments >> 1) & 1);  // b
    lcd_set_ram_bit((com_seg_map + seg_c)->com, (com_seg_map + seg_c)->seg, (segments >> 2) & 1);  // c
    lcd_set_ram_bit((com_seg_map + seg_d)->com, (com_seg_map + seg_d)->seg, (segments >> 3) & 1);  // d
    lcd_set_ram_bit((com_seg_map + seg_e)->com, (com_seg_map + seg_e)->seg, (segments >> 4) & 1);  // e
    lcd_set_ram_bit((com_seg_map + seg_f)->com, (com_seg_map + seg_f)->seg, (segments >> 5) & 1);  // f
    lcd_set_ram_bit((com_seg_map + seg_g)->com, (com_seg_map + seg_g)->seg, (segments >> 6) & 1);  // g
    
    // Set decimal point if required
    if(show_dp) {
        lcd_set_ram_bit((com_seg_map + seg_dp)->com, (com_seg_map + seg_dp)->seg, 1);
    } else {
        lcd_set_ram_bit((com_seg_map + seg_dp)->com, (com_seg_map + seg_dp)->seg, 0);
    }
}

/**
  * @brief  Display character segments at a position (backward compatibility)
 * @param  position: Digit position (0-5)
  * @param  segments: Segment pattern
  * @retval None
  */
void lcd_display_char_segments(uint8_t position, uint8_t segments)
{
    lcd_display_char_segments_with_dp(position, segments, 0);  // Don't show decimal point by default
}

/**
 * @brief  Show plus/minus symbol based on value
 * @param  value: 1 for plus, -1 for minus, 0 for off
 * @retval None
 */
void lcd_show_plus_minus_symbols(int8_t value)
{
 // Clear both symbols first
 lcd_clear_segment(lcd_segment_com_seg_map[LCD_SEG_SIGN].com, lcd_segment_com_seg_map[LCD_SEG_SIGN].seg);
 lcd_clear_segment(lcd_segment_com_seg_map[LCD_SEG_POSITIVE].com, lcd_segment_com_seg_map[LCD_SEG_POSITIVE].seg);
 
 if(value > 0) {
   // Show plus sign (combination of SIGN and POSITIVE fields)
   lcd_set_segment(lcd_segment_com_seg_map[LCD_SEG_SIGN].com, lcd_segment_com_seg_map[LCD_SEG_SIGN].seg);
   lcd_set_segment(lcd_segment_com_seg_map[LCD_SEG_POSITIVE].com, lcd_segment_com_seg_map[LCD_SEG_POSITIVE].seg);
 } else if(value < 0) {
   // Show minus sign (SIGN field only - LCD_SEG_SIGN which is COM3/SEG0)
   lcd_set_segment(lcd_segment_com_seg_map[LCD_SEG_SIGN].com, lcd_segment_com_seg_map[LCD_SEG_SIGN].seg);
 }
 // If value == 0, both symbols remain off
}

/**
 * @brief  Show a specific symbol with state
 * @param  symbol: Symbol to show (from lcd_segment_t)
 * @param  state: 1 to show, 0 to hide
 * @retval None
 * @note   This is a consolidated function for showing/hiding symbols, reducing redundancy
 */
void lcd_set_symbol_state(lcd_segment_t symbol, uint8_t state)
{
   // Use the existing lcd_show_symbol function which already handles the mapping correctly
   lcd_show_symbol(symbol, state);
}

/**
 * @brief  Show multiple symbols at once using a unified approach
 * @param  symbols: Array of symbols to control
 * @param  states: Array of states (0 or 1) for each symbol
 * @param  count: Number of symbols to control
 * @retval None
 * @note   This function consolidates multiple symbol display operations into a single function
 */
void lcd_show_multiple_symbols(lcd_segment_t *symbols, uint8_t *states, uint8_t count)
{
 for(uint8_t i = 0; i < count; i++) {
   lcd_show_symbol(symbols[i], states[i]);  // Using array indexing instead of pointer arithmetic
 }
}

/**
 * @brief Show MAX/MIN/TIR symbols
 * @param  max_active: 1 to show MAX, 0 to hide
 * @param  min_active: 1 to show MIN, 0 to hide
 * @param  tir_active: 1 to show TIR, 0 to hide
 * @retval None
 */
void lcd_show_max_min_tir_symbols(uint8_t max_active, uint8_t min_active, uint8_t tir_active)
{
 lcd_segment_t symbols[] = {LCD_SEG_MAX, LCD_SEG_MIN, LCD_SEG_TIR};
 uint8_t states[] = {max_active, min_active, tir_active};
 lcd_show_multiple_symbols(symbols, states, 3);
}

/**
 * @brief  Show OK/NG symbols
 * @param  ok_active: 1 to show OK, 0 to hide
 * @param  ng_active: 1 to show NG, 0 to hide
 * @retval None
 */
void lcd_show_ok_ng_symbols(uint8_t ok_active, uint8_t ng_active)
{
 lcd_segment_t symbols[] = {LCD_SEG_OK, LCD_SEG_NG};
 uint8_t states[] = {ok_active, ng_active};
 lcd_show_multiple_symbols(symbols, states, 2);
}

/**
 * @brief  Show unit symbols (mm/inch)
 * @param  mm_active: 1 to show mm, 0 to hide
 * @param  in_active: 1 to show inch, 0 to hide
 * @retval None
 */
void lcd_show_unit_symbols(uint8_t mm_active, uint8_t in_active)
{
 lcd_segment_t symbols[] = {LCD_SEG_MM_P, LCD_SEG_IN};
 uint8_t states[] = {mm_active, in_active};
 lcd_show_multiple_symbols(symbols, states, 2);
}

/**
 * @brief  Show tolerance symbol
 * @param  active: 1 to show tol, 0 to hide
 * @retval None
 */
void lcd_show_tolerance_symbol(uint8_t active)
{
 lcd_show_symbol(LCD_SEG_TOL, active);
}

/**
 * @brief  Show relative measurement symbol
 * @param  active: 1 to show rel, 0 to hide
 * @retval None
 */
void lcd_show_relative_symbol(uint8_t active)
{
 lcd_show_symbol(LCD_SEG_REL, active);
}

/**
 * @brief  Show hold symbol
 * @param  active: 1 to show hold, 0 to hide
 * @retval None
 */
void lcd_show_hold_symbol(uint8_t active)
{
 lcd_show_symbol(LCD_SEG_HOLD, active);
}

/**
 * @brief  Show low battery symbol
 * @param  active: 1 to show lo-batt, 0 to hide
 * @retval None
 */
void lcd_show_low_battery_symbol(uint8_t active)
{
 lcd_show_symbol(LCD_SEG_LO_BATT, active);
}

/**
  * @brief  Show a specific symbol
  * @param  symbol: Symbol to show (from lcd_segment_t)
  * @param  state: 1 to show, 0 to hide
  * @retval None
  */
   /**
    * @brief  Show a specific symbol
    * @param  symbol: Symbol to show (from lcd_segment_t)
    * @param  state: 1 to show, 0 to hide
    * @retval None
    */
   void lcd_show_symbol(lcd_segment_t symbol, uint8_t state)
   {
       uint8_t com_line, seg_line;
       
       // Use the unified mapping function to get COM and SEG values
       if(lcd_get_com_from_segment(symbol, &com_line, &seg_line)) {
           if(state) {
               lcd_set_segment(com_line, seg_line);
           } else {
               lcd_clear_segment(com_line, seg_line);
           }
       }
       // If the symbol is invalid, do nothing
   }
   
   /**
 * @brief  Display a string (limited character set)
 * @param  str: String to display
 * @retval None
 */
void lcd_display_string(const char* str)
{
    // Clear LCD first before displaying new string
    lcd_clear_all();
    
    if(str == NULL) return;
    
    // Preprocess the string to identify decimal points and associate them with preceding digits
    char display_chars[6] = {0};  // Characters to display in each position
    uint8_t show_dp[6] = {0};    // Which positions should show decimal point
    
    const char *str_ptr = str;
    uint8_t char_idx = 0;  // Index in the input string
    uint8_t pos = 0;       // Position on the display (0-5)
    
    // Process the input string to extract characters and decimal point positions
    while(*str_ptr && pos < 6 && char_idx < 6) {
        if(*str_ptr == '.' && char_idx > 0 && pos > 0) {
            // This is a decimal point that should be shown on the previous digit
            // Find the last displayed digit position to add decimal point to
            show_dp[pos-1] = 1;
        } else if (*str_ptr != '.') {
            // Regular character (not a decimal point)
            display_chars[pos] = *str_ptr;
            pos++;  // Move to next display position
        }
        
        str_ptr++;
        char_idx++;
    }
    
    // Now display each character with its associated decimal point setting
    for(uint8_t i = 0; i < 6; i++) {  // Process all 6 positions to ensure old content is cleared
        if(i < pos && display_chars[i] != '\0') { // Only display if we have a character for this position
            // Check if this character is a digit and we need to show decimal point
            if(display_chars[i] >= '0' && display_chars[i] <= '9' && show_dp[i]) {
                lcd_display_digit_with_dp(i, display_chars[i] - '0', 1);
            } else if (display_chars[i] >= '0' && display_chars[i] <= '9') {
                lcd_display_digit_with_dp(i, display_chars[i] - '0', 0);
            } else {
                // For non-digits, show the character normally
                // If decimal point is needed for this position, we'll need to handle it separately
                lcd_display_char(i, display_chars[i]);
                
                // If this position needs a decimal point, force it on after character display
                if(show_dp[i]) {
                    // Get the segment mapping for this position and enable the decimal point
                    const lcd_digit_segments_t *seg_map = &digit_seg_map[i];
                    lcd_set_ram_bit(lcd_segment_com_seg_map[seg_map->seg_dp].com,
                                    lcd_segment_com_seg_map[seg_map->seg_dp].seg, 1);
                }
            }
        }
        // If no character is assigned to this position (i >= pos), it remains cleared
    }
}

/* USER CODE BEGIN 1 */

/**
 * @brief  Clear all LCD symbols (all segments except digits)
  * @retval None
  * @note   This function clears all symbol segments on the LCD, preserving digit content temporarily
  *         The display update is handled by the underlying lcd_show_symbol function calls
  */
void lcd_clear_all_symbols(void)
{
    // Iterate through all possible LCD segments and clear them if they're symbols (not digit segments)
    for (int i = LCD_SEG_REV; i < LCD_SEG_MAX_ENUM; i++) {
        // Skip digit segments (these are handled separately)
        // Digit segments are: A, B, C, D, E, F, G, DP for each digit position
        // These correspond to specific ranges in our enum, so we clear all other symbols
        lcd_segment_t symbol = (lcd_segment_t)i;
        lcd_show_symbol(symbol, 0);  // Turn off all symbols
    }
    
    // Force immediate update to ensure all symbols are cleared on display
    HAL_LCD_UpdateDisplayRequest(&hlcd);
}

/**
* @brief Show PS (Position Scale) symbols based on global analog pointer value
* @param last_digit: Last digit of the displayed number (0-9) - unused, use global analog_pointer_value instead
* @param sign: Sign of the number (1 for negative, 0 for positive) - unused, use global analog_pointer_value instead
* @retval None
*/
void lcd_show_ps_symbols(uint8_t last_digit, uint8_t sign)
{
   // Use global analog pointer value instead of parameters
   extern int8_t analog_pointer_value;
   
   // First, clear all PS-related symbols except PS0 which should always be on
   // Clear PS++ (ps++)
   lcd_show_symbol(LCD_SEG_PS_PP, 0);
   // Clear PS+1 to PS+15
   lcd_show_symbol(LCD_SEG_PS_P1, 0);
   lcd_show_symbol(LCD_SEG_PS_P2, 0);
   lcd_show_symbol(LCD_SEG_PS_P3, 0);
   lcd_show_symbol(LCD_SEG_PS_P4, 0);
   lcd_show_symbol(LCD_SEG_PS_P5, 0);
   lcd_show_symbol(LCD_SEG_PS_P6, 0);
   lcd_show_symbol(LCD_SEG_PS_P7, 0);
   lcd_show_symbol(LCD_SEG_PS_P8, 0);
   lcd_show_symbol(LCD_SEG_PS_P9, 0);
   lcd_show_symbol(LCD_SEG_PS_P10, 0);
   lcd_show_symbol(LCD_SEG_PS_P11, 0);
   lcd_show_symbol(LCD_SEG_PS_P12, 0);
   lcd_show_symbol(LCD_SEG_PS_P13, 0);
   lcd_show_symbol(LCD_SEG_PS_P14, 0);
   lcd_show_symbol(LCD_SEG_PS_P15, 0);
   // Clear PS-1 to PS-15
   lcd_show_symbol(LCD_SEG_PS_N1, 0);
   lcd_show_symbol(LCD_SEG_PS_N2, 0);
   lcd_show_symbol(LCD_SEG_PS_N3, 0);
   lcd_show_symbol(LCD_SEG_PS_N4, 0);
   lcd_show_symbol(LCD_SEG_PS_N5, 0);
   lcd_show_symbol(LCD_SEG_PS_N6, 0);
   lcd_show_symbol(LCD_SEG_PS_N7, 0);
   lcd_show_symbol(LCD_SEG_PS_N8, 0);
   lcd_show_symbol(LCD_SEG_PS_N9, 0);
   lcd_show_symbol(LCD_SEG_PS_N10, 0);
   lcd_show_symbol(LCD_SEG_PS_N11, 0);
   lcd_show_symbol(LCD_SEG_PS_N12, 0);
   lcd_show_symbol(LCD_SEG_PS_N13, 0);
   lcd_show_symbol(LCD_SEG_PS_N14, 0);
   lcd_show_symbol(LCD_SEG_PS_N15, 0);
   // Clear PS-- (ps--)
   lcd_show_symbol(LCD_SEG_PS_MM, 0);

   // PS0 should always be on when showing analog pointer
   lcd_show_symbol(LCD_SEG_PS_0, 1);

   // Use the global analog pointer value to control the PS symbols
   if (analog_pointer_value > 15) {
       lcd_show_symbol(LCD_SEG_PS_PP, 1);  // Show positive zone icon (ps++)
       
       // Show all positive indicators up to 15 (full bar)
       for (int i = 1; i <= 15; i++) {
           switch(i) {
               case 1: lcd_show_symbol(LCD_SEG_PS_P1, 1); break;
               case 2: lcd_show_symbol(LCD_SEG_PS_P2, 1); break;
               case 3: lcd_show_symbol(LCD_SEG_PS_P3, 1); break;
               case 4: lcd_show_symbol(LCD_SEG_PS_P4, 1); break;
               case 5: lcd_show_symbol(LCD_SEG_PS_P5, 1); break;
               case 6: lcd_show_symbol(LCD_SEG_PS_P6, 1); break;
               case 7: lcd_show_symbol(LCD_SEG_PS_P7, 1); break;
               case 8: lcd_show_symbol(LCD_SEG_PS_P8, 1); break;
               case 9: lcd_show_symbol(LCD_SEG_PS_P9, 1); break;
               case 10: lcd_show_symbol(LCD_SEG_PS_P10, 1); break;
               case 11: lcd_show_symbol(LCD_SEG_PS_P11, 1); break;
               case 12: lcd_show_symbol(LCD_SEG_PS_P12, 1); break;
               case 13: lcd_show_symbol(LCD_SEG_PS_P13, 1); break;
               case 14: lcd_show_symbol(LCD_SEG_PS_P14, 1); break;
               case 15: lcd_show_symbol(LCD_SEG_PS_P15, 1); break;
           }
       }
   }
   else if (analog_pointer_value > 0) {
       lcd_show_symbol(LCD_SEG_PS_PP, 1);  // Show positive zone icon (ps++)
       
       // Show positive indicators up to the analog pointer value (columnar display)
       for (int i = 1; i <= analog_pointer_value; i++) {
           switch(i) {
               case 1: lcd_show_symbol(LCD_SEG_PS_P1, 1); break;
               case 2: lcd_show_symbol(LCD_SEG_PS_P2, 1); break;
               case 3: lcd_show_symbol(LCD_SEG_PS_P3, 1); break;
               case 4: lcd_show_symbol(LCD_SEG_PS_P4, 1); break;
               case 5: lcd_show_symbol(LCD_SEG_PS_P5, 1); break;
               case 6: lcd_show_symbol(LCD_SEG_PS_P6, 1); break;
               case 7: lcd_show_symbol(LCD_SEG_PS_P7, 1); break;
               case 8: lcd_show_symbol(LCD_SEG_PS_P8, 1); break;
               case 9: lcd_show_symbol(LCD_SEG_PS_P9, 1); break;
               case 10: lcd_show_symbol(LCD_SEG_PS_P10, 1); break;
               case 11: lcd_show_symbol(LCD_SEG_PS_P11, 1); break;
               case 12: lcd_show_symbol(LCD_SEG_PS_P12, 1); break;
               case 13: lcd_show_symbol(LCD_SEG_PS_P13, 1); break;
               case 14: lcd_show_symbol(LCD_SEG_PS_P14, 1); break;
               case 15: lcd_show_symbol(LCD_SEG_PS_P15, 1); break;
           }
       }
   }
   else if (analog_pointer_value == 0) {
       // For zero value, only show PS0 (center reference)
       // Already handled by keeping PS0 on and not activating others
   }
   else if (analog_pointer_value < -15) {
       lcd_show_symbol(LCD_SEG_PS_MM, 1);  // Show negative zone icon (ps--)
       
       // Show all negative indicators down to -15 (full bar)
       for (int i = 1; i <= 15; i++) {
           switch(i) {
               case 1: lcd_show_symbol(LCD_SEG_PS_N1, 1); break;
               case 2: lcd_show_symbol(LCD_SEG_PS_N2, 1); break;
               case 3: lcd_show_symbol(LCD_SEG_PS_N3, 1); break;
               case 4: lcd_show_symbol(LCD_SEG_PS_N4, 1); break;
               case 5: lcd_show_symbol(LCD_SEG_PS_N5, 1); break;
               case 6: lcd_show_symbol(LCD_SEG_PS_N6, 1); break;
               case 7: lcd_show_symbol(LCD_SEG_PS_N7, 1); break;
               case 8: lcd_show_symbol(LCD_SEG_PS_N8, 1); break;
               case 9: lcd_show_symbol(LCD_SEG_PS_N9, 1); break;
               case 10: lcd_show_symbol(LCD_SEG_PS_N10, 1); break;
               case 11: lcd_show_symbol(LCD_SEG_PS_N11, 1); break;
               case 12: lcd_show_symbol(LCD_SEG_PS_N12, 1); break;
               case 13: lcd_show_symbol(LCD_SEG_PS_N13, 1); break;
               case 14: lcd_show_symbol(LCD_SEG_PS_N14, 1); break;
               case 15: lcd_show_symbol(LCD_SEG_PS_N15, 1); break;
           }
       }
   }
   else { // analog_pointer_value < 0
       lcd_show_symbol(LCD_SEG_PS_MM, 1);  // Show negative zone icon (ps--)
       
       // Show negative indicators down to the absolute value of analog pointer value (columnar display)
       for (int i = 1; i <= -analog_pointer_value; i++) {
           switch(i) {
               case 1: lcd_show_symbol(LCD_SEG_PS_N1, 1); break;
               case 2: lcd_show_symbol(LCD_SEG_PS_N2, 1); break;
               case 3: lcd_show_symbol(LCD_SEG_PS_N3, 1); break;
               case 4: lcd_show_symbol(LCD_SEG_PS_N4, 1); break;
               case 5: lcd_show_symbol(LCD_SEG_PS_N5, 1); break;
               case 6: lcd_show_symbol(LCD_SEG_PS_N6, 1); break;
               case 7: lcd_show_symbol(LCD_SEG_PS_N7, 1); break;
               case 8: lcd_show_symbol(LCD_SEG_PS_N8, 1); break;
               case 9: lcd_show_symbol(LCD_SEG_PS_N9, 1); break;
               case 10: lcd_show_symbol(LCD_SEG_PS_N10, 1); break;
               case 11: lcd_show_symbol(LCD_SEG_PS_N11, 1); break;
               case 12: lcd_show_symbol(LCD_SEG_PS_N12, 1); break;
               case 13: lcd_show_symbol(LCD_SEG_PS_N13, 1); break;
               case 14: lcd_show_symbol(LCD_SEG_PS_N14, 1); break;
               case 15: lcd_show_symbol(LCD_SEG_PS_N15, 1); break;
           }
       }
   }
}

/* USER CODE END 1 */

/**
  * @brief  Display sensor data using the SensorData_TypeDef structure
  * @param sensor_data: Pointer to the SensorData_TypeDef structure containing sensor data
 * @retval None
  * @note   This function displays sensor data directly from the structure, eliminating need for data conversion
  *         Now includes decimal point handling for mm unit display
  */
void lcd_display_sensor_data_from_struct(const SensorData_TypeDef *sensor_data)
{
   // Prepare display string (using the structure data)
   char display_str[7] = {0};  // 6 digits + null terminator
   
   for(int i = 0; i < 6; i++) {
       if(sensor_data->digits[i] <= 9) {
           display_str[i] = '0' + sensor_data->digits[i];
       } else {
           display_str[i] = '0';  // Default to '0' for invalid values
       }
   }
   
   // Clear LCD first to ensure clean state
   lcd_clear_all();
   
   // Update LCD with the digits, adding decimal point at the correct position
   for(int i = 0; i < 6; i++) {
       if(display_str[i] >= '0' && display_str[i] <= '9') {
           // Add decimal point after the 2nd digit from left (on the 2nd digit itself, index 1)
           // This shows format like XX.YYYY for millimeter display with 4 decimal places
           uint8_t show_decimal_point = 0;
           if (i == 0) {  // Place decimal point after 1st digit from left (i=0), so format shows as X.YYYYY
               show_decimal_point = 1;
           }
           
           // Directly call digit display function to ensure RAM is updated
           lcd_display_digit_with_dp(i, display_str[i] - '0', show_decimal_point);
       }
   }
   
   // Handle unit and sign display based on specification
   // NIB5第4位控制公制/英制 (unit_bit) - 1为英制显示"in"，0为公制显示"mm"
   if(sensor_data->unit_metric) {
       // 1为英制（显示LCD图标"in"）
       lcd_show_unit_symbols(0, 1);  // Deactivate mm, activate in
   } else {
       // 0为公制（显示LCD图标"mm"）
       lcd_show_unit_symbols(1, 0);  // Activate mm, deactivate in
   }
   
   // NIB5第2位控制正负号 (sign_bit) - 0为正号(不显示)，1为负号(显示"-")
   if(sensor_data->sign == 0) {
       // 正数，不显示符号（已默认处理）
   } else {
       // 负数，显示负号
       lcd_show_plus_minus_symbols(-1);  // Show minus sign
   }
   
   // Show PS symbols based on the last digit and sign
   // Only show PS symbols based on sensor data if not using global analog pointer
   extern int8_t analog_pointer_value;  // 引用全局模拟指针值
   // For now, we'll still show PS symbols based on sensor data
   lcd_show_ps_symbols(sensor_data->last_digit, sensor_data->sign);
   
   // Force immediate display update to ensure data appears on LCD
   lcd_force_update_display();
}

/**
  * @brief  Update LCD based on the latest sensor data
  * @retval None
  * @note   This function retrieves the latest sensor data and updates the display
  */
void lcd_update_from_sensor_data(void)
{
    // Define the global sensor data structure
    SensorData_TypeDef sensor_data = {0};  // Initialize all fields to 0
    lcd_display_sensor_data_from_struct(&sensor_data);
}

/**
  * @brief  Force update the LCD display with clear-and-write approach
  * @retval None
  * @note   This function requests immediate update of the LCD display from hardware registers
  *         This ensures the hardware registers are properly updated
  */
void lcd_force_update_display(void)
{
    // Request display update to show current content after all changes are written
    HAL_LCD_UpdateDisplayRequest(&hlcd);
    last_update_tick = HAL_GetTick();  // Update timestamp
}

/**
  * @brief  Prepare LCD for STOP mode entry (preserve display content)
  * @param  None
  * @retval None
  */
void LCD_PrepareForStopMode(void)
{
    // Ensure LCD display is properly latched before entering STOP mode
    // The STM32L1 LCD controller can maintain display content in low-power modes
    // but we need to ensure the charge sharing circuitry is properly configured
    
    // Disable the charge pump if needed, or configure for low leakage
    // In STOP mode, the LCD controller can maintain display content if properly configured
    // No special action needed here since the LCD RAM content persists in STOP mode
}

/**
  * @brief  Restore LCD after STOP mode exit
  * @param  None
  * @retval None
  */
void LCD_RestoreAfterStopMode(void)
{
    // After waking from STOP mode, the LCD controller needs to be re-enabled
    // The LCD RAM content should still be intact, but we need to ensure
    // the LCD controller is properly configured and the display is refreshed
    
    // Re-enable the LCD controller if it was disabled
    __HAL_LCD_ENABLE(&hlcd);
    
    // Request a display refresh to ensure content is visible
    HAL_LCD_UpdateDisplayRequest(&hlcd);

}
