#include "pwm_input_capture.h"

typedef struct
{
  volatile uint32_t period_us;
  volatile uint16_t pulse_width_us;
  volatile uint32_t last_update_ms;
  volatile uint8_t has_sample;
} PwmInputCaptureChannel;

static PwmInputCaptureChannel channels[2];

static HAL_StatusTypeDef startTimer(TIM_HandleTypeDef *htim)
{
  HAL_StatusTypeDef status = HAL_TIM_IC_Start(htim, TIM_CHANNEL_2);
  if (status != HAL_OK)
  {
    return status;
  }

  status = HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1);
  if (status != HAL_OK)
  {
    (void)HAL_TIM_IC_Stop(htim, TIM_CHANNEL_2);
  }
  return status;
}

HAL_StatusTypeDef PwmInputCapture_Start(TIM_HandleTypeDef *htim3,
                                        TIM_HandleTypeDef *htim4)
{
  HAL_StatusTypeDef status;
  if ((htim3 == NULL) || (htim4 == NULL) ||
      (htim3->Instance != TIM3) || (htim4->Instance != TIM4))
  {
    return HAL_ERROR;
  }

  status = startTimer(htim3);
  if (status != HAL_OK)
  {
    return status;
  }

  status = startTimer(htim4);
  if (status != HAL_OK)
  {
    (void)HAL_TIM_IC_Stop_IT(htim3, TIM_CHANNEL_1);
    (void)HAL_TIM_IC_Stop(htim3, TIM_CHANNEL_2);
  }
  return status;
}

void PwmInputCapture_Read(PwmInputCaptureSnapshot snapshots[2])
{
  const uint32_t primask = __get_PRIMASK();
  __disable_irq();
  for (uint32_t channel = 0U; channel < 2U; ++channel)
  {
    snapshots[channel].period_us = channels[channel].period_us;
    snapshots[channel].pulse_width_us = channels[channel].pulse_width_us;
    snapshots[channel].last_update_ms = channels[channel].last_update_ms;
    snapshots[channel].has_sample = channels[channel].has_sample;
  }
  if (primask == 0U)
  {
    __enable_irq();
  }
}

void PwmInputCapture_OnInterrupt(TIM_HandleTypeDef *htim)
{
  uint32_t channel_index;
  if ((htim == NULL) || (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1))
  {
    return;
  }

  if (htim->Instance == TIM3)
  {
    channel_index = 0U;
  }
  else if (htim->Instance == TIM4)
  {
    channel_index = 1U;
  }
  else
  {
    return;
  }

  const uint32_t period_us =
    HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
  const uint32_t pulse_width_us =
    HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
  if ((period_us == 0U) || (pulse_width_us > period_us) ||
      (pulse_width_us > UINT16_MAX))
  {
    return;
  }

  PwmInputCaptureChannel *capture = &channels[channel_index];
  capture->period_us = period_us;
  capture->pulse_width_us = (uint16_t)pulse_width_us;
  capture->last_update_ms = HAL_GetTick();
  capture->has_sample = 1U;
}
