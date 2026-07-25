#include "main.h"
#include "App_Control.h"
#include "IO_Init.h"

static void APP_SystemClockConfig(void);
static void APP_ConfigureOptionBytes(void);

int main(void)
{
  HAL_Init();
  APP_SystemClockConfig();
  APP_ConfigureOptionBytes();

  IO_Init();
  APP_Control_Init();

  while (1)
  {
    APP_Control_Task();
  }
}

static void APP_ConfigureOptionBytes(void)
{
  FLASH_OBProgramInitTypeDef option_bytes = {0};

  HAL_FLASH_OBGetConfig(&option_bytes);
  if ((option_bytes.USERConfig & OB_USER_SWD_NRST_MODE) == OB_SWD_PB6_GPIO_PC0)
  {
    return;
  }

  option_bytes.OptionType = OPTIONBYTE_USER;
  option_bytes.USERType = OB_USER_SWD_NRST_MODE;
  option_bytes.USERConfig = OB_SWD_PB6_GPIO_PC0;

  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    APP_ErrorHandler();
  }

  if (HAL_FLASH_OB_Unlock() != HAL_OK)
  {
    HAL_FLASH_Lock();
    APP_ErrorHandler();
  }

  if (HAL_FLASH_OBProgram(&option_bytes) != HAL_OK)
  {
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    APP_ErrorHandler();
  }

  if (HAL_FLASH_OB_Lock() != HAL_OK)
  {
    HAL_FLASH_Lock();
    APP_ErrorHandler();
  }

  if (HAL_FLASH_Lock() != HAL_OK)
  {
    APP_ErrorHandler();
  }

  HAL_FLASH_OB_Launch();
  APP_ErrorHandler();
}

static void APP_SystemClockConfig(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_24MHz;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSISYS;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    APP_ErrorHandler();
  }
}

void APP_ErrorHandler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
