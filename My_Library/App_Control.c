#include "App_Control.h"
#include "ADC_Feedback.h"
#include "Timer1.h"
#include "Timer14.h"

#define PWM_SOFT_START_FREQ_HZ       75000U
#define PWM_NORMAL_RUN_FREQ_HZ       40000U
#define PWM_SOFT_START_STEP_FREQ_HZ   1000U
#define PWM_SOFT_START_INTERVAL_MS     100U
#define PWM_SPECIAL_START_MS          2100U
#define PWM_SPECIAL_END_MS            2400U
#define PWM_SPECIAL_DELAY_MS           300U
#define PWM_SOFT_START_DURATION_MS    3800U
#define PWM_FAULT_FREQ_HZ              5700U

typedef enum
{
  APP_STATE_NORMAL = 0,
  APP_STATE_FAULT
} APP_StateTypeDef;

static volatile uint32_t app_tick_ms;

static APP_StateTypeDef app_state;
static uint32_t normal_start_tick_ms;

static void APP_Control_EnterNormal(void);
static void APP_Control_EnterFault(void);
static void APP_Control_UpdateNormalOutput(uint32_t now_ms);
static void APP_Control_ApplyFeedbackState(ADC_FeedbackStateTypeDef feedback_state);

void APP_Control_Init(void)
{
  app_tick_ms = 0U;

  TIM14_Timebase_Init();
  TIM14_Timebase_Start();

  ADC_Feedback_Init();
  TIM1_PWM_Init(PWM_SOFT_START_FREQ_HZ);
  APP_Control_EnterNormal();
}

void APP_Control_Task(void)
{
  uint32_t now_ms = APP_Control_GetTickMs();
  ADC_FeedbackStateTypeDef feedback_state = ADC_Feedback_Task(now_ms);

  APP_Control_ApplyFeedbackState(feedback_state);

  if (app_state == APP_STATE_NORMAL)
  {
    APP_Control_UpdateNormalOutput(now_ms);
  }
}

void APP_Control_AddTickMs(uint32_t elapsed_ms)
{
  app_tick_ms += elapsed_ms;
}

uint32_t APP_Control_GetTickMs(void)
{
  return app_tick_ms;
}

static void APP_Control_EnterNormal(void)
{
  app_state = APP_STATE_NORMAL;
  normal_start_tick_ms = APP_Control_GetTickMs();
  TIM1_PWM_SetFrequency(PWM_SOFT_START_FREQ_HZ);
}

static void APP_Control_EnterFault(void)
{
  app_state = APP_STATE_FAULT;
  TIM1_PWM_SetFrequency(PWM_FAULT_FREQ_HZ);
}

static void APP_Control_UpdateNormalOutput(uint32_t now_ms)
{
  uint32_t elapsed_ms = now_ms - normal_start_tick_ms;
  uint32_t target_frequency_hz;
  uint32_t delayed_elapsed_ms;

  if (elapsed_ms < PWM_SPECIAL_START_MS)
  {
    target_frequency_hz = PWM_SOFT_START_FREQ_HZ -
                          ((elapsed_ms / PWM_SOFT_START_INTERVAL_MS) *
                           PWM_SOFT_START_STEP_FREQ_HZ);
  }
  else if (elapsed_ms < 2140U)
  {
    target_frequency_hz = 2500U;
  }
  else if (elapsed_ms < 2180U)
  {
    target_frequency_hz = 1200U;
  }
  else if (elapsed_ms < 2220U)
  {
    target_frequency_hz = 600U;
  }
  else if (elapsed_ms < 2280U)
  {
    target_frequency_hz = 200U;
  }
  else if (elapsed_ms < 2320U)
  {
    target_frequency_hz = 600U;
  }
  else if (elapsed_ms < 2360U)
  {
    target_frequency_hz = 1200U;
  }
  else if (elapsed_ms < PWM_SPECIAL_END_MS)
  {
    target_frequency_hz = 2500U;
  }
  else if (elapsed_ms < PWM_SOFT_START_DURATION_MS)
  {
    delayed_elapsed_ms = elapsed_ms - PWM_SPECIAL_DELAY_MS;
    target_frequency_hz = PWM_SOFT_START_FREQ_HZ -
                          ((delayed_elapsed_ms / PWM_SOFT_START_INTERVAL_MS) *
                           PWM_SOFT_START_STEP_FREQ_HZ);
  }
  else
  {
    target_frequency_hz = PWM_NORMAL_RUN_FREQ_HZ;
  }

  TIM1_PWM_SetFrequency(target_frequency_hz);
}

static void APP_Control_ApplyFeedbackState(ADC_FeedbackStateTypeDef feedback_state)
{
  if ((feedback_state == ADC_FEEDBACK_STATE_FAULT) && (app_state != APP_STATE_FAULT))
  {
    APP_Control_EnterFault();
  }
  else if ((feedback_state == ADC_FEEDBACK_STATE_NORMAL) && (app_state != APP_STATE_NORMAL))
  {
    APP_Control_EnterNormal();
  }
}
