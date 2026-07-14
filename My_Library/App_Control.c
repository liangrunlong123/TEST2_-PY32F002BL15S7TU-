#include "App_Control.h"
#include "Timer1.h"
#include "Timer14.h"

#define PWM_NORMAL_START_FREQ_HZ   70000U
#define PWM_NORMAL_RUN_FREQ_HZ     39000U
#define PWM_FAULT_FREQ_HZ          5500U
#define PWM_STEP_FREQ_HZ           1000U
#define PWM_STEP_INTERVAL_MS       100U
#define FAULT_DETECT_ENABLE_MS     5000U

typedef enum
{
  APP_STATE_NORMAL = 0,
  APP_STATE_FAULT
} APP_StateTypeDef;

static volatile uint32_t app_tick_ms;
static volatile uint8_t fault_edge_pending;
static volatile GPIO_PinState fault_edge_state;

static APP_StateTypeDef app_state;
static uint32_t last_step_tick_ms;
static uint32_t pwm_frequency_hz;
static uint8_t fault_detection_enabled;

static void APP_Control_EnterNormal(void);
static void APP_Control_EnterFault(void);
static void APP_Control_UpdateNormalRamp(uint32_t now_ms);
static void APP_Control_ApplyFaultPinState(GPIO_PinState pin_state);
static void APP_Control_EnableFaultEdgeDetection(void);

void APP_Control_Init(void)
{
  app_tick_ms = 0U;
  fault_edge_pending = 0U;
  fault_edge_state = GPIO_PIN_SET;
  fault_detection_enabled = 0U;

  TIM14_Timebase_Init();
  TIM14_Timebase_Start();

  TIM1_PWM_Init(PWM_NORMAL_START_FREQ_HZ);
  APP_Control_EnterNormal();
}

void APP_Control_Task(void)
{
  uint32_t now_ms = APP_Control_GetTickMs();

  if (fault_detection_enabled == 0U)
  {
    APP_Control_UpdateNormalRamp(now_ms);

    if (now_ms >= FAULT_DETECT_ENABLE_MS)
    {
      fault_detection_enabled = 1U;
      APP_Control_ApplyFaultPinState(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7));
      APP_Control_EnableFaultEdgeDetection();
      APP_Control_ApplyFaultPinState(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7));
    }

    return;
  }

  if (fault_edge_pending != 0U)
  {
    __disable_irq();
    GPIO_PinState edge_state = fault_edge_state;
    fault_edge_pending = 0U;
    __enable_irq();

    APP_Control_ApplyFaultPinState(edge_state);
  }

  if (app_state == APP_STATE_NORMAL)
  {
    APP_Control_UpdateNormalRamp(now_ms);
  }
}

void APP_Control_OnFaultSignalEdge(GPIO_PinState pin_state)
{
  if (fault_detection_enabled != 0U)
  {
    fault_edge_state = pin_state;
    fault_edge_pending = 1U;
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
  uint32_t now_ms = APP_Control_GetTickMs();

  app_state = APP_STATE_NORMAL;
  last_step_tick_ms = now_ms;
  pwm_frequency_hz = PWM_NORMAL_START_FREQ_HZ;
  TIM1_PWM_SetFrequency(pwm_frequency_hz);
}

static void APP_Control_EnterFault(void)
{
  app_state = APP_STATE_FAULT;
  pwm_frequency_hz = PWM_FAULT_FREQ_HZ;
  TIM1_PWM_SetFrequency(pwm_frequency_hz);
}

static void APP_Control_UpdateNormalRamp(uint32_t now_ms)
{
  while (((now_ms - last_step_tick_ms) >= PWM_STEP_INTERVAL_MS) &&
         (pwm_frequency_hz > PWM_NORMAL_RUN_FREQ_HZ))
  {
    last_step_tick_ms += PWM_STEP_INTERVAL_MS;

    if ((pwm_frequency_hz - PWM_STEP_FREQ_HZ) < PWM_NORMAL_RUN_FREQ_HZ)
    {
      pwm_frequency_hz = PWM_NORMAL_RUN_FREQ_HZ;
    }
    else
    {
      pwm_frequency_hz -= PWM_STEP_FREQ_HZ;
    }

    TIM1_PWM_SetFrequency(pwm_frequency_hz);
  }
}

static void APP_Control_ApplyFaultPinState(GPIO_PinState pin_state)
{
  if ((pin_state == GPIO_PIN_RESET) && (app_state != APP_STATE_FAULT))
  {
    APP_Control_EnterFault();
  }
  else if ((pin_state == GPIO_PIN_SET) && (app_state != APP_STATE_NORMAL))
  {
    APP_Control_EnterNormal();
  }
}

static void APP_Control_EnableFaultEdgeDetection(void)
{
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, PRIORITY_LOW, 0U);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}
