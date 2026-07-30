#include "App_Control.h"
#include "ADC_Feedback.h"
#include "Timer1.h"
#include "Timer14.h"

#define PWM_SOFT_START_FREQ_HZ       75000U
#define PWM_NORMAL_RUN_FREQ_HZ       40000U
#define PWM_SOFT_START_STEP_FREQ_HZ    500U
#define PWM_SOFT_START_INTERVAL_MS      50U
#define PWM_SOFT_START_DURATION_MS    3500U
#define PWM_HIGH_VOLTAGE_BOOST_HZ      5000U

typedef enum
{
  APP_STATE_NORMAL = 0,
  APP_STATE_FAULT
} APP_StateTypeDef;

typedef enum
{
  PWM_OUTPUT_PHASE_SOFT_START = 0,
  PWM_OUTPUT_PHASE_BOOSTED_FIXED
} PWM_OutputPhaseTypeDef;

static volatile uint32_t app_tick_ms;

static APP_StateTypeDef app_state;
static PWM_OutputPhaseTypeDef pwm_output_phase;
static uint32_t normal_start_tick_ms;
static uint32_t current_normal_frequency_hz;
static uint32_t boosted_frequency_hz;

static void APP_Control_EnterNormal(void);
static void APP_Control_EnterFault(void);
static void APP_Control_UpdateNormalOutput(uint32_t now_ms, uint8_t high_voltage_confirmed);
static void APP_Control_ApplyFeedbackState(ADC_FeedbackStateTypeDef feedback_state);
static uint32_t APP_Control_CalculateHighFrequency(uint32_t elapsed_ms);

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
  uint8_t high_voltage_confirmed;

  APP_Control_ApplyFeedbackState(feedback_state);
  high_voltage_confirmed = ADC_Feedback_IsHighVoltageConfirmed();

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
  pwm_output_phase = PWM_OUTPUT_PHASE_SOFT_START;
  normal_start_tick_ms = APP_Control_GetTickMs();
  current_normal_frequency_hz = PWM_SOFT_START_FREQ_HZ;
  boosted_frequency_hz = 0U;
  ADC_Feedback_ResetHighVoltageConfirmation();
  TIM1_PWM_SetFrequency(PWM_SOFT_START_FREQ_HZ);
}

static void APP_Control_EnterFault(void)
{
  app_state = APP_STATE_FAULT;
  TIM1_PWM_EnterFaultOutput();
}

static void APP_Control_UpdateNormalOutput(uint32_t now_ms, uint8_t high_voltage_confirmed)
{
  uint32_t elapsed_ms = now_ms - normal_start_tick_ms;
  uint32_t target_frequency_hz = APP_Control_CalculateHighFrequency(elapsed_ms);

  if ((pwm_output_phase == PWM_OUTPUT_PHASE_SOFT_START) &&
      (elapsed_ms < PWM_SOFT_START_DURATION_MS) &&
      (high_voltage_confirmed != 0U))
  {
    boosted_frequency_hz = current_normal_frequency_hz + PWM_HIGH_VOLTAGE_BOOST_HZ;
    pwm_output_phase = PWM_OUTPUT_PHASE_BOOSTED_FIXED;
  }

  if (pwm_output_phase == PWM_OUTPUT_PHASE_BOOSTED_FIXED)
  {
    target_frequency_hz = boosted_frequency_hz;
  }
  else
  {
    current_normal_frequency_hz = target_frequency_hz;
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
