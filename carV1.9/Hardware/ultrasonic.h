#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void Ultrasonic_Init(void);
uint32_t Ultrasonic_ReadDistanceCm(void);

#ifdef __cplusplus
}
#endif

#endif
