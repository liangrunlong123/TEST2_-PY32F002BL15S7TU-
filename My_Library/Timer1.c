#include "Timer1.h"

#define TIM1_PWM_CHANNEL        TIM_CHANNEL_2
#define PWM_DUTY_NUMERATOR      1U
#define PWM_DUTY_DENOMINATOR    2U

static TIM_HandleTypeDef htim1;
static uint32_t current_frequency_hz;
static uint32_t TIM1_PWM_CalculatePeriod(uint32_t frequency_hz);

void TIM1_PWM_Init(uint32_t frequency_hz)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  uint32_t period = TIM1_PWM_CalculatePeriod(frequency_hz);

  __HAL_RCC_TIM1_CLK_ENABLE();

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0U;
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
}

void TIM1_PWM_SetFrequency(uint32_t frequency_hz)
{
  uint32_t period;
  uint32_t pulse;

  if (frequency_hz == current_frequency_hz)
  {
    return;
  }

  period = TIM1_PWM_CalculatePeriod(frequency_hz);
  pulse = ((period + 1U) * PWM_DUTY_NUMERATOR) / PWM_DUTY_DENOMINATOR;

  __disable_irq();
  __HAL_TIM_SET_AUTORELOAD(&htim1, period);
  __HAL_TIM_SET_COMPARE(&htim1, TIM1_PWM_CHANNEL, pulse);
  current_frequency_hz = frequency_hz;
  __enable_irq();
}

void TIM1_Service_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim1);
}

static uint32_t TIM1_PWM_CalculatePeriod(uint32_t frequency_hz)
{
  uint32_t timer_clock_hz = HAL_RCC_GetHCLKFreq();

  if (frequency_hz == 0U)
  {
    APP_ErrorHandler();
  }

  return ((timer_clock_hz + (frequency_hz / 2U)) / frequency_hz) - 1U;
}
