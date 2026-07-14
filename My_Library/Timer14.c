#include "Timer14.h"
#include "App_Control.h"

#define TIM14_TIMEBASE_MS  10U

static TIM_HandleTypeDef htim14;

void TIM14_Timebase_Init(void)
{
  __HAL_RCC_TIM14_CLK_ENABLE();

  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 24U - 1U;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = (1000U * TIM14_TIMEBASE_MS) - 1U;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  HAL_NVIC_SetPriority(TIM14_IRQn, PRIORITY_LOW, 0U);
  HAL_NVIC_EnableIRQ(TIM14_IRQn);
}

void TIM14_Timebase_Start(void)
{
  if (HAL_TIM_Base_Start_IT(&htim14) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

void TIM14_Timebase_Stop(void)
{
  if (HAL_TIM_Base_Stop_IT(&htim14) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

void TIM14_Service_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim14);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM14)
  {
    APP_Control_AddTickMs(TIM14_TIMEBASE_MS);
  }
}
