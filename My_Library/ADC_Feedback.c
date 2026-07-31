#include "ADC_Feedback.h"

#define ADC_FEEDBACK_SAMPLE_INTERVAL_MS   5U
#define ADC_FEEDBACK_FIRST_SAMPLE_MS      10U
#define ADC_FEEDBACK_FIRST_DECISION_MS    50U
#define ADC_FEEDBACK_FILTER_SIZE          9U
#define ADC_REFERENCE_MV                  5000U
#define ADC_FAULT_THRESHOLD_MV            560U
#define ADC_HIGH_VOLTAGE_THRESHOLD_MV     4850U
#define ADC_MAX_RAW_VALUE                 4095U
#define ADC_CONVERSION_TIMEOUT_MS         2U
#define ADC_STATE_CONFIRMATION_COUNT       2U
#define ADC_HIGH_VOLTAGE_CONFIRMATION_COUNT 7U

#define ADC_FAULT_THRESHOLD_RAW \
  (((ADC_FAULT_THRESHOLD_MV * ADC_MAX_RAW_VALUE) + (ADC_REFERENCE_MV / 2U)) / ADC_REFERENCE_MV)
#define ADC_HIGH_VOLTAGE_THRESHOLD_RAW \
  (((ADC_HIGH_VOLTAGE_THRESHOLD_MV * ADC_MAX_RAW_VALUE) + ADC_REFERENCE_MV - 1U) / ADC_REFERENCE_MV)

static ADC_HandleTypeDef hadc_feedback;
static uint16_t adc_samples[ADC_FEEDBACK_FILTER_SIZE];
static uint8_t adc_sample_index;
static uint8_t adc_sample_count;
static uint32_t next_sample_tick_ms;
static uint16_t median_raw;
static ADC_FeedbackStateTypeDef feedback_state;
static ADC_FeedbackStateTypeDef candidate_state;
static uint8_t candidate_state_count;
static uint8_t high_voltage_sample_count;

static uint16_t ADC_Feedback_ReadRaw(void);
static uint16_t ADC_Feedback_CalculateMedian(void);
static void ADC_Feedback_PushSample(uint16_t sample);
static void ADC_Feedback_ConfirmState(ADC_FeedbackStateTypeDef observed_state);
static void ADC_Feedback_UpdateHighVoltageCount(uint16_t sample);

void ADC_Feedback_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  __HAL_RCC_ADC_CLK_ENABLE();

  hadc_feedback.Instance = ADC1;
  hadc_feedback.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc_feedback.Init.Resolution = ADC_RESOLUTION_12B;
  hadc_feedback.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc_feedback.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc_feedback.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc_feedback.Init.LowPowerAutoWait = DISABLE;
  hadc_feedback.Init.ContinuousConvMode = DISABLE;
  hadc_feedback.Init.DiscontinuousConvMode = DISABLE;
  hadc_feedback.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc_feedback.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc_feedback.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  hadc_feedback.Init.SamplingTimeCommon = ADC_SAMPLETIME_239CYCLES_5;

  if (HAL_ADC_Init(&hadc_feedback) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  if (HAL_ADC_ConfigVrefBuf(&hadc_feedback, ADC_VREFBUF_VCCA) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.Channel = ADC_CHANNEL_4;
  if (HAL_ADC_ConfigChannel(&hadc_feedback, &sConfig) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  if (HAL_ADCEx_Calibration_Start(&hadc_feedback) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  adc_sample_index = 0U;
  adc_sample_count = 0U;
  next_sample_tick_ms = ADC_FEEDBACK_FIRST_SAMPLE_MS;
  median_raw = 0U;
  feedback_state = ADC_FEEDBACK_STATE_UNKNOWN;
  candidate_state = ADC_FEEDBACK_STATE_UNKNOWN;
  candidate_state_count = 0U;
  high_voltage_sample_count = 0U;
}

ADC_FeedbackStateTypeDef ADC_Feedback_Task(uint32_t now_ms)
{
  uint16_t sample;

  if ((int32_t)(now_ms - next_sample_tick_ms) < 0)
  {
    return feedback_state;
  }

  next_sample_tick_ms = now_ms + ADC_FEEDBACK_SAMPLE_INTERVAL_MS;
  sample = ADC_Feedback_ReadRaw();
  ADC_Feedback_PushSample(sample);
  ADC_Feedback_UpdateHighVoltageCount(sample);

  if ((now_ms < ADC_FEEDBACK_FIRST_DECISION_MS) ||
      (adc_sample_count < ADC_FEEDBACK_FILTER_SIZE))
  {
    feedback_state = ADC_FEEDBACK_STATE_UNKNOWN;
    return feedback_state;
  }

  median_raw = ADC_Feedback_CalculateMedian();

  if (median_raw > ADC_FAULT_THRESHOLD_RAW)
  {
    ADC_Feedback_ConfirmState(ADC_FEEDBACK_STATE_NORMAL);
  }
  else
  {
    ADC_Feedback_ConfirmState(ADC_FEEDBACK_STATE_FAULT);
  }

  return feedback_state;
}

uint16_t ADC_Feedback_GetMedianRaw(void)
{
  return median_raw;
}

uint8_t ADC_Feedback_IsHighVoltageConfirmed(void)
{
  return (high_voltage_sample_count >= ADC_HIGH_VOLTAGE_CONFIRMATION_COUNT) ? 1U : 0U;
}

void ADC_Feedback_ResetHighVoltageConfirmation(void)
{
  high_voltage_sample_count = 0U;
}

static uint16_t ADC_Feedback_ReadRaw(void)
{
  uint16_t value;

  if (HAL_ADC_Start(&hadc_feedback) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  if (HAL_ADC_PollForConversion(&hadc_feedback, ADC_CONVERSION_TIMEOUT_MS) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  value = (uint16_t)HAL_ADC_GetValue(&hadc_feedback);

  if (HAL_ADC_Stop(&hadc_feedback) != HAL_OK)
  {
    APP_ErrorHandler();
  }

  return value;
}

static void ADC_Feedback_PushSample(uint16_t sample)
{
  adc_samples[adc_sample_index] = sample;
  adc_sample_index++;

  if (adc_sample_index >= ADC_FEEDBACK_FILTER_SIZE)
  {
    adc_sample_index = 0U;
  }

  if (adc_sample_count < ADC_FEEDBACK_FILTER_SIZE)
  {
    adc_sample_count++;
  }
}

static uint16_t ADC_Feedback_CalculateMedian(void)
{
  uint16_t sorted[ADC_FEEDBACK_FILTER_SIZE];
  uint8_t i;
  uint8_t j;

  for (i = 0U; i < ADC_FEEDBACK_FILTER_SIZE; i++)
  {
    sorted[i] = adc_samples[i];
  }

  for (i = 1U; i < ADC_FEEDBACK_FILTER_SIZE; i++)
  {
    uint16_t key = sorted[i];
    j = i;

    while ((j > 0U) && (sorted[j - 1U] > key))
    {
      sorted[j] = sorted[j - 1U];
      j--;
    }

    sorted[j] = key;
  }

  return sorted[ADC_FEEDBACK_FILTER_SIZE / 2U];
}

static void ADC_Feedback_ConfirmState(ADC_FeedbackStateTypeDef observed_state)
{
  if (observed_state == feedback_state)
  {
    candidate_state = ADC_FEEDBACK_STATE_UNKNOWN;
    candidate_state_count = 0U;
    return;
  }

  if (observed_state != candidate_state)
  {
    candidate_state = observed_state;
    candidate_state_count = 1U;
  }
  else if (candidate_state_count < ADC_STATE_CONFIRMATION_COUNT)
  {
    candidate_state_count++;
  }

  if (candidate_state_count >= ADC_STATE_CONFIRMATION_COUNT)
  {
    feedback_state = candidate_state;
    candidate_state = ADC_FEEDBACK_STATE_UNKNOWN;
    candidate_state_count = 0U;
  }
}

static void ADC_Feedback_UpdateHighVoltageCount(uint16_t sample)
{
  if (sample >= ADC_HIGH_VOLTAGE_THRESHOLD_RAW)
  {
    if (high_voltage_sample_count < ADC_HIGH_VOLTAGE_CONFIRMATION_COUNT)
    {
      high_voltage_sample_count++;
    }
  }
  else
  {
    high_voltage_sample_count = 0U;
  }
}
