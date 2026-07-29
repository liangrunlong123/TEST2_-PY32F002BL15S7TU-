#include "App_Control.h"
#include "ADC_Feedback.h"
#include "Timer1.h"
#include "Timer14.h"

#define PWM_SOFT_START_FREQ_HZ       75000U
#define PWM_NORMAL_RUN_FREQ_HZ       40000U
#define PWM_SOFT_START_STEP_FREQ_HZ   1000U
#define PWM_SOFT_START_INTERVAL_MS     100U
#define PWM_LOW_FREQ_REQUEST_START_MS 2050U
#define PWM_LOW_FREQ_REQUEST_END_MS   2450U
#define PWM_SOFT_START_DURATION_MS    3500U
#define PWM_FAULT_FREQ_HZ              5700U

typedef enum
{
  APP_STATE_NORMAL = 0,
  APP_STATE_FAULT
} APP_StateTypeDef;

typedef enum
{
  PWM_START_PHASE_HIGH_FREQUENCY = 0,
  PWM_START_PHASE_TRIGGERED_SEQUENCE
} PWM_StartPhaseTypeDef;

static volatile uint32_t app_tick_ms;

static APP_StateTypeDef app_state;
static PWM_StartPhaseTypeDef pwm_start_phase;
static uint32_t normal_start_tick_ms;
static uint32_t low_frequency_start_tick_ms;

static void APP_Control_EnterNormal(void);
static void APP_Control_EnterFault(void);
static void APP_Control_UpdateNormalOutput(uint32_t now_ms, uint8_t high_voltage_confirmed);
static void APP_Control_ApplyFeedbackState(ADC_FeedbackStateTypeDef feedback_state);
static uint32_t APP_Control_CalculateHighFrequency(uint32_t elapsed_ms);
static uint32_t APP_Control_CalculateTriggeredFrequency(uint32_t elapsed_ms);

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
  uint8_t high_voltage_confirmed = ADC_Feedback_IsHighVoltageConfirmed();

  APP_Control_ApplyFeedbackState(feedback_state);

  if (app_state == APP_STATE_NORMAL)
  {
    APP_Control_UpdateNormalOutput(now_ms, high_voltage_confirmed);
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
  pwm_start_phase = PWM_START_PHASE_HIGH_FREQUENCY;
  normal_start_tick_ms = APP_Control_GetTickMs();
  low_frequency_start_tick_ms = 0U;
  TIM1_PWM_SetFrequency(PWM_SOFT_START_FREQ_HZ);
}

static void APP_Control_EnterFault(void)
{
  app_state = APP_STATE_FAULT;
  TIM1_PWM_SetFrequency(PWM_FAULT_FREQ_HZ);
}

static void APP_Control_UpdateNormalOutput(uint32_t now_ms, uint8_t high_voltage_confirmed)
{
  uint32_t elapsed_ms = now_ms - normal_start_tick_ms;
  uint32_t triggered_elapsed_ms;
  uint32_t target_frequency_hz;

  if ((pwm_start_phase == PWM_START_PHASE_HIGH_FREQUENCY) &&
      (elapsed_ms >= PWM_LOW_FREQ_REQUEST_START_MS) &&
      (elapsed_ms <= PWM_LOW_FREQ_REQUEST_END_MS) &&
      (high_voltage_confirmed != 0U))
  {
    pwm_start_phase = PWM_START_PHASE_TRIGGERED_SEQUENCE;
    low_frequency_start_tick_ms = now_ms;
  }

  if (pwm_start_phase == PWM_START_PHASE_TRIGGERED_SEQUENCE)
  {
    triggered_elapsed_ms = now_ms - low_frequency_start_tick_ms;
    target_frequency_hz = APP_Control_CalculateTriggeredFrequency(triggered_elapsed_ms);
  }
  else
  {
    target_frequency_hz = APP_Control_CalculateHighFrequency(elapsed_ms);
  }

  TIM1_PWM_SetFrequency(target_frequency_hz);
}

static uint32_t APP_Control_CalculateHighFrequency(uint32_t elapsed_ms)
{
  if (elapsed_ms < PWM_SOFT_START_DURATION_MS)
  {
    return PWM_SOFT_START_FREQ_HZ -
           ((elapsed_ms / PWM_SOFT_START_INTERVAL_MS) * PWM_SOFT_START_STEP_FREQ_HZ);
  }

  return PWM_NORMAL_RUN_FREQ_HZ;
}

static uint32_t APP_Control_CalculateTriggeredFrequency(uint32_t elapsed_ms)
{
  if (elapsed_ms < 30U)
  {
    return 2500U;
  }
  if (elapsed_ms < 60U)
  {
    return 1600U;
  }
  if (elapsed_ms < 90U)
  {
    return 1000U;
  }
  if (elapsed_ms < 120U)
  {
    return 400U;
  }
  if (elapsed_ms < 180U)
  {
    return 200U;
  }
  if (elapsed_ms < 210U)
  {
    return 400U;
  }
  if (elapsed_ms < 240U)
  {
    return 1000U;
  }
  if (elapsed_ms < 270U)
  {
    return 1600U;
  }
  if (elapsed_ms < 300U)
  {
    return 2500U;
  }
  if (elapsed_ms < 400U)
  {
    return 45000U;
  }
  if (elapsed_ms < 500U)
  {
    return 44000U;
  }
  if (elapsed_ms < 600U)
  {
    return 43000U;
  }
  if (elapsed_ms < 700U)
  {
    return 42000U;
  }
  if (elapsed_ms < 800U)
  {
    return 41000U;
  }

  return PWM_NORMAL_RUN_FREQ_HZ;
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
