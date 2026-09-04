#include "hall_capture.h"

typedef struct
{
  volatile uint32_t previous_tick;
  volatile uint32_t period_ticks;
  volatile uint32_t last_pulse_ms;
  volatile uint8_t has_pulse;
  volatile uint8_t has_period;
} HallCaptureChannel;

static HallCaptureChannel channels[2];

HAL_StatusTypeDef HallCapture_Start(TIM_HandleTypeDef *htim)
{
  HAL_StatusTypeDef status = HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1);
  if (status != HAL_OK)
  {
    return status;
  }

  status = HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_2);
  if (status != HAL_OK)
  {
    (void)HAL_TIM_IC_Stop_IT(htim, TIM_CHANNEL_1);
  }
  return status;
}

void HallCapture_Read(HallCaptureSnapshot snapshots[2])
{
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  for (uint32_t channel = 0U; channel < 2U; ++channel)
  {
    snapshots[channel].period_ticks = channels[channel].period_ticks;
    snapshots[channel].last_pulse_ms = channels[channel].last_pulse_ms;
    snapshots[channel].has_pulse = channels[channel].has_pulse;
    snapshots[channel].has_period = channels[channel].has_period;
  }
  if (primask == 0U)
  {
    __enable_irq();
  }
}

void HallCapture_OnInterrupt(TIM_HandleTypeDef *htim)
{
  uint32_t channel_index;
  uint32_t timer_channel;

  if ((htim == NULL) || (htim->Instance != TIM2))
  {
    return;
  }

  if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    channel_index = 0U;
    timer_channel = TIM_CHANNEL_1;
  }
  else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    channel_index = 1U;
    timer_channel = TIM_CHANNEL_2;
  }
  else
  {
    return;
  }

  HallCaptureChannel *capture = &channels[channel_index];
  const uint32_t timer_tick = HAL_TIM_ReadCapturedValue(htim, timer_channel);
  if (capture->has_pulse != 0U)
  {
    capture->period_ticks = timer_tick - capture->previous_tick;
    capture->has_period = 1U;
  }
  capture->previous_tick = timer_tick;
  capture->last_pulse_ms = HAL_GetTick();
  capture->has_pulse = 1U;
}
