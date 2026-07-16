#include "App_Control.h"
#include "ADC_Feedback.h"
#include "Timer1.h"
#include "Timer14.h"

#define PWM_STAGE1_START_FREQ_HZ    2500U
#define PWM_STAGE1_STEP_FREQ_HZ      500U
#define PWM_STAGE1_STEP_INTERVAL_MS   50U
#define PWM_STAGE1_DURATION_MS       350U
#define PWM_STAGE2_START_FREQ_HZ   70000U
#define PWM_STAGE2_RUN_FREQ_HZ     40000U
#define PWM_STAGE2_STEP_FREQ_HZ     1000U
#define PWM_STAGE2_STEP_INTERVAL_MS  100U
#define PWM_STAGE2_DURATION_MS      3000U
#define PWM_FAULT_FREQ_HZ            5700U

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
  TIM1_PWM_Init(PWM_STAGE1_START_FREQ_HZ);
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
  TIM1_PWM_SetFrequency(PWM_STAGE1_START_FREQ_HZ);
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

  if (elapsed_ms < PWM_STAGE1_DURATION_MS)
  {
    target_frequency_hz = PWM_STAGE1_START_FREQ_HZ +
                          ((elapsed_ms / PWM_STAGE1_STEP_INTERVAL_MS) *
                           PWM_STAGE1_STEP_FREQ_HZ);
  }
  else if (elapsed_ms < (PWM_STAGE1_DURATION_MS + PWM_STAGE2_DURATION_MS))
  {
    uint32_t stage2_elapsed_ms = elapsed_ms - PWM_STAGE1_DURATION_MS;

    target_frequency_hz = PWM_STAGE2_START_FREQ_HZ -
                          ((stage2_elapsed_ms / PWM_STAGE2_STEP_INTERVAL_MS) *
                           PWM_STAGE2_STEP_FREQ_HZ);
  }
  else
  {
    target_frequency_hz = PWM_STAGE2_RUN_FREQ_HZ;
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
