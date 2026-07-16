#ifndef __ADC_FEEDBACK_H
#define __ADC_FEEDBACK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

typedef enum
{
  ADC_FEEDBACK_STATE_UNKNOWN = 0,
  ADC_FEEDBACK_STATE_NORMAL,
  ADC_FEEDBACK_STATE_FAULT
} ADC_FeedbackStateTypeDef;

void ADC_Feedback_Init(void);
ADC_FeedbackStateTypeDef ADC_Feedback_Task(uint32_t now_ms);
uint16_t ADC_Feedback_GetMedianRaw(void);

#ifdef __cplusplus
}
#endif

#endif
