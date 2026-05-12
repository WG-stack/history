#ifndef __BUZZER_H
#define __BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_BeepSlowTwice(void);
void Buzzer_BeepFastTwice(void);

#ifdef __cplusplus
}
#endif

#endif
