/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    lcd_segment_mapping.h
  * @brief   LCD Segment Mapping Documentation based on provided segment table
  ******************************************************************************
  * @attention
  *
  * This file documents the LCD segment mapping based on the provided segment table.
  * 
  * SEG/COM	COM1	COM2	COM3	COM4
  * SEG0	'rev'	'fwd'	'sign'	'Positive'
  * SEG1	1F	1G	1E	1D
  * SEG2	1A	1B	1C	'p1'
  * SEG3	2F	2G	2E	2D
  * SEG4	2A	2B	2C	'p2'
  * SEG5	3F	3G	3E	3D
  * SEG6	3A	3B	3C	'p3'
  * SEG7	4F	4G	4E	4D
  * SEG8	4A	4B	4C	'p4'
  * SEG9	5F	5G	5E	5D
  * SEG10	5A	5B	5C	'p5'
  * SEG11	6F	6G	6E	6D
  * SEG12	6A	6B	6C	'set'
  * SEG13	'ok'	'NG'	'tol'	'rel'
  * SEG14	'max'	TIR	'min'	'hold'
  * SEG15	'ins1'	'inu1'	'in-p'	'in'
  * SEG16	'mm'	'mm-p'	'mmst'	'mmut'
  * SEG17	'lo-batt'	'ps++'	'ps+15'	'ps0'
  * SEG18	'ps+14'	'ps+13'	'ps+12'	'ps+11'
  * SEG19	'ps+10'	'ps+9'	'ps+8'	'ps+7'
  * SEG20	'ps+6'	'ps+5'	'ps+4'	'ps+3'
  * SEG21	'ps+2'	'ps+1'	'ps-1'	'ps-2'
  * SEG22	'ps-3'	'ps-4'	'ps-5'	'ps-6'
  * SEG23	'ps-7'	'ps-8'	'ps-9'	'ps-10'
  * SEG24	'ps-11'	'ps-12'	'ps-13'	'ps-14'
  * SEG25	'ps--'	'ps-15'	--	--
  *
  * PS+1~PS+15 对应正号模拟指针，超 + 15 时显示 “ps++”；
  * PS-1~PS-15 对应负号模拟指针，超 - 15 时显示 “ps--”；
  * PS0 为常显状态，用于基准指示
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LCD_SEGMENT_MAPPING_H__
#define __LCD_SEGMENT_MAPPING_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Define segment mapping based on the provided table */
#define LCD_COM1_SEG0     'rev'     // REV indicator
#define LCD_COM2_SEG0     'fwd'     // FWD indicator  
#define LCD_COM3_SEG0     'sign'    // SIGN indicator
#define LCD_COM4_SEG0     'Positive' // Positive indicator

/* Digit 1 segments */
#define LCD_COM1_SEG1     1F        // Digit 1F segment
#define LCD_COM2_SEG1     1G        // Digit 1G segment
#define LCD_COM3_SEG1     1E        // Digit 1E segment
#define LCD_COM4_SEG1     1D        // Digit 1D segment

#define LCD_COM1_SEG2     1A        // Digit 1A segment
#define LCD_COM2_SEG2     1B        // Digit 1B segment
#define LCD_COM3_SEG2     1C        // Digit 1C segment
#define LCD_COM4_SEG2     'p1'      // Digit 1 decimal point

/* Digit 2 segments */
#define LCD_COM1_SEG3     2F        // Digit 2F segment
#define LCD_COM2_SEG3     2G        // Digit 2G segment
#define LCD_COM3_SEG3     2E        // Digit 2E segment
#define LCD_COM4_SEG3     2D        // Digit 2D segment

#define LCD_COM1_SEG4     2A        // Digit 2A segment
#define LCD_COM2_SEG4     2B        // Digit 2B segment
#define LCD_COM3_SEG4     2C        // Digit 2C segment
#define LCD_COM4_SEG4     'p2'      // Digit 2 decimal point

/* Digit 3 segments */
#define LCD_COM1_SEG5     3F        // Digit 3F segment
#define LCD_COM2_SEG5     3G        // Digit 3G segment
#define LCD_COM3_SEG5     3E        // Digit 3E segment
#define LCD_COM4_SEG5     3D        // Digit 3D segment

#define LCD_COM1_SEG6     3A        // Digit 3A segment
#define LCD_COM2_SEG6     3B        // Digit 3B segment
#define LCD_COM3_SEG6     3C        // Digit 3C segment
#define LCD_COM4_SEG6     'p3'      // Digit 3 decimal point

/* Digit 4 segments */
#define LCD_COM1_SEG7     4F        // Digit 4F segment
#define LCD_COM2_SEG7     4G        // Digit 4G segment
#define LCD_COM3_SEG7     4E        // Digit 4E segment
#define LCD_COM4_SEG7     4D        // Digit 4D segment

#define LCD_COM1_SEG8     4A        // Digit 4A segment
#define LCD_COM2_SEG8     4B        // Digit 4B segment
#define LCD_COM3_SEG8     4C        // Digit 4C segment
#define LCD_COM4_SEG8     'p4'      // Digit 4 decimal point

/* Digit 5 segments */
#define LCD_COM1_SEG9     5F        // Digit 5F segment
#define LCD_COM2_SEG9     5G        // Digit 5G segment
#define LCD_COM3_SEG9     5E        // Digit 5E segment
#define LCD_COM4_SEG9     5D        // Digit 5D segment

#define LCD_COM1_SEG10    5A        // Digit 5A segment
#define LCD_COM2_SEG10    5B        // Digit 5B segment
#define LCD_COM3_SEG10    5C        // Digit 5C segment
#define LCD_COM4_SEG10    'p5'      // Digit 5 decimal point

/* Digit 6 segments */
#define LCD_COM1_SEG11    6F        // Digit 6F segment
#define LCD_COM2_SEG11    6G        // Digit 6G segment
#define LCD_COM3_SEG11    6E        // Digit 6E segment
#define LCD_COM4_SEG11    6D        // Digit 6D segment

#define LCD_COM1_SEG12    6A        // Digit 6A segment
#define LCD_COM2_SEG12    6B        // Digit 6B segment
#define LCD_COM3_SEG12    6C        // Digit 6C segment
#define LCD_COM4_SEG12    'set'     // Digit 6 decimal point / SET indicator

/* Control symbols */
#define LCD_COM1_SEG13    'ok'      // OK indicator
#define LCD_COM2_SEG13    'NG'      // NG indicator
#define LCD_COM3_SEG13    'tol'     // Tolerance indicator
#define LCD_COM4_SEG13    'rel'     // Relative indicator

#define LCD_COM1_SEG14    'max'     // MAX indicator
#define LCD_COM2_SEG14    'TIR'     // TIR indicator
#define LCD_COM3_SEG14    'min'     // MIN indicator
#define LCD_COM4_SEG14    'hold'    // HOLD indicator

#define LCD_COM1_SEG15    'ins1'    // INS1 indicator
#define LCD_COM2_SEG15    'inu1'    //INU1 indicator
#define LCD_COM3_SEG15    'in-p'    // IN-P indicator
#define LCD_COM4_SEG15    'in'      // IN indicator

#define LCD_COM1_SEG16    'mm'      // MM indicator
#define LCD_COM2_SEG16    'mm-p'    // MM-P indicator
#define LCD_COM3_SEG16    'mmst'    // MMST indicator
#define LCD_COM4_SEG16    'mmut'    // MMUT indicator

/* Power and PS indicators */
#define LCD_COM1_SEG17    'lo-batt' // Low battery indicator
#define LCD_COM2_SEG17    'ps++'    // PS++ indicator (over +15)
#define LCD_COM3_SEG17    'ps+15'   // PS+15 indicator
#define LCD_COM4_SEG17    'ps0'     // PS0 indicator (baseline)

#define LCD_COM1_SEG18    'ps+14'   // PS+14 indicator
#define LCD_COM2_SEG18    'ps+13'   // PS+13 indicator
#define LCD_COM3_SEG18    'ps+12'   // PS+12 indicator
#define LCD_COM4_SEG18    'ps+11'   // PS+11 indicator

#define LCD_COM1_SEG19    'ps+10'   // PS+10 indicator
#define LCD_COM2_SEG19    'ps+9'    // PS+9 indicator
#define LCD_COM3_SEG19    'ps+8'    // PS+8 indicator
#define LCD_COM4_SEG19    'ps+7'    // PS+7 indicator

#define LCD_COM1_SEG20    'ps+6'    // PS+6 indicator
#define LCD_COM2_SEG20    'ps+5'    // PS+5 indicator
#define LCD_COM3_SEG20    'ps+4'    // PS+4 indicator
#define LCD_COM4_SEG20    'ps+3'    // PS+3 indicator

#define LCD_COM1_SEG21    'ps+2'    // PS+2 indicator
#define LCD_COM2_SEG21    'ps+1'    // PS+1 indicator
#define LCD_COM3_SEG21    'ps-1'    // PS-1 indicator
#define LCD_COM4_SEG21    'ps-2'    // PS-2 indicator

#define LCD_COM1_SEG22    'ps-3'    // PS-3 indicator
#define LCD_COM2_SEG22    'ps-4'    // PS-4 indicator
#define LCD_COM3_SEG22    'ps-5'    // PS-5 indicator
#define LCD_COM4_SEG22    'ps-6'    // PS-6 indicator

#define LCD_COM1_SEG23    'ps-7'    // PS-7 indicator
#define LCD_COM2_SEG23    'ps-8'    // PS-8 indicator
#define LCD_COM3_SEG23    'ps-9'    // PS-9 indicator
#define LCD_COM4_SEG23    'ps-10'   // PS-10 indicator

#define LCD_COM1_SEG24    'ps-11'   // PS-11 indicator
#define LCD_COM2_SEG24    'ps-12'   // PS-12 indicator
#define LCD_COM3_SEG24    'ps-13'   // PS-13 indicator
#define LCD_COM4_SEG24    'ps-14'   // PS-14 indicator

#define LCD_COM1_SEG25    'ps--'    // PS-- indicator (under -15)
#define LCD_COM2_SEG25    'ps-15'   // PS-15 indicator
/* COM3/SEG25 and COM4/SEG25 are unused (--) */

/* PS Analog Pointer Range Definitions */
#define PS_POINTER_MIN_VALUE    -15   /*!< Minimum PS pointer value */
#define PS_POINTER_MAX_VALUE    15    /*!< Maximum PS pointer value */
#define PS_POINTER_OVERFLOW_POS "ps++"/*!< Display when value > 15 */
#define PS_POINTER_OVERFLOW_NEG "ps--"/*!< Display when value < -15 */
#define PS_POINTER_BASELINE     "ps0" /*!< Baseline indicator (always on) */

#ifdef __cplusplus
}
#endif

#endif /* __LCD_SEGMENT_MAPPING_H__ */