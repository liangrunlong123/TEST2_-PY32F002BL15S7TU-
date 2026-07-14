#ifndef __TIMER14_H
#define __TIMER14_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void TIM14_Timebase_Init(void);
void TIM14_Timebase_Start(void);
void TIM14_Timebase_Stop(void);
void TIM14_Service_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif
