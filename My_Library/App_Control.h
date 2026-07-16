#ifndef __APP_CONTROL_H
#define __APP_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void APP_Control_Init(void);
void APP_Control_Task(void);
void APP_Control_AddTickMs(uint32_t elapsed_ms);
uint32_t APP_Control_GetTickMs(void);

#ifdef __cplusplus
}
#endif

#endif
