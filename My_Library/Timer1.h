#ifndef __TIMER1_H
#define __TIMER1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void TIM1_PWM_Init(uint32_t frequency_hz);
void TIM1_PWM_SetFrequency(uint32_t frequency_hz);

#ifdef __cplusplus
}
#endif

#endif
