#include "Timer1.h"

#define TIM1_PWM_CHANNEL        TIM_CHANNEL_2
#define TIM1_COUNTER_COUNTS     65536U
#define PWM_DUTY_NUMERATOR      1U
#define PWM_DUTY_DENOMINATOR    2U
#define PWM_FAULT_FREQUENCY_HZ  5700U

static TIM_HandleTypeDef htim1;
static uint32_t current_frequency_hz;
static volatile uint8_t pwm_started;
static void TIM1_PWM_CalculateTiming(uint32_t frequency_hz,
                                    uint32_t *prescaler,
                                    uint32_t *period);

void TIM1_PWM_Init(uint32_t frequency_hz)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  uint32_t prescaler;
  uint32_t period;

  pwm_started = 0U;
  TIM1_PWM_CalculateTiming(frequency_hz, &prescaler, &period);

  __HAL_RCC_TIM1_CLK_ENABLE();

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = prescaler;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = period;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0U;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = ((period + 1U) * PWM_DUTY_NUMERATOR) / PWM_DUTY_DENOMINATOR;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM1_PWM_CHANNEL) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  if (HAL_TIM_PWM_Start(&htim1, TIM1_PWM_CHANNEL) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  current_frequency_hz = frequency_hz;
  pwm_started = 1U;
}

void TIM1_PWM_SetFrequency(uint32_t frequency_hz)
{
  uint32_t prescaler;
  uint32_t period;
  uint32_t pulse;
  uint32_t update_disable_state;
  uint32_t primask;

  if (frequency_hz == current_frequency_hz)
  {
    return;
  }

  TIM1_PWM_CalculateTiming(frequency_hz, &prescaler, &period);
  pulse = ((period + 1U) * PWM_DUTY_NUMERATOR) / PWM_DUTY_DENOMINATOR;

  primask = __get_PRIMASK();
  __disable_irq();
  update_disable_state = READ_BIT(htim1.Instance->CR1, TIM_CR1_UDIS);
  SET_BIT(htim1.Instance->CR1, TIM_CR1_UDIS);
  __HAL_TIM_SET_PRESCALER(&htim1, prescaler);
  __HAL_TIM_SET_AUTORELOAD(&htim1, period);
  __HAL_TIM_SET_COMPARE(&htim1, TIM1_PWM_CHANNEL, pulse);

  if (update_disable_state == 0U)
  {
    CLEAR_BIT(htim1.Instance->CR1, TIM_CR1_UDIS);
  }

  current_frequency_hz = frequency_hz;
  if (primask == 0U)
  {
    __enable_irq();
  }
}

void TIM1_PWM_EnterFaultOutput(void)
{
  if (pwm_started != 0U)
  {
    TIM1_PWM_SetFrequency(PWM_FAULT_FREQUENCY_HZ);
  }
}

static void TIM1_PWM_CalculateTiming(uint32_t frequency_hz,
                                    uint32_t *prescaler,
                                    uint32_t *period)
{
  uint32_t timer_clock_hz = HAL_RCC_GetHCLKFreq();
  uint32_t timer_counts;
  uint32_t prescaler_divider;
  uint32_t counter_clock_hz;

  if ((frequency_hz == 0U) || (frequency_hz > timer_clock_hz))
  {
    APP_ErrorHandler();
  }

  timer_counts = (timer_clock_hz + (frequency_hz / 2U)) / frequency_hz;
  prescaler_divider = ((timer_counts - 1U) / TIM1_COUNTER_COUNTS) + 1U;
  counter_clock_hz = timer_clock_hz / prescaler_divider;

  *prescaler = prescaler_divider - 1U;
  *period = ((counter_clock_hz + (frequency_hz / 2U)) / frequency_hz) - 1U;
}
