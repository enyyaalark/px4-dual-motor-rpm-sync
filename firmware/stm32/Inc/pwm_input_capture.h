#ifndef PWM_INPUT_CAPTURE_H
#define PWM_INPUT_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "stm32g4xx_hal.h"

typedef struct
{
  uint32_t period_us;
  uint16_t pulse_width_us;
  uint32_t last_update_ms;
  uint8_t has_sample;
} PwmInputCaptureSnapshot;

HAL_StatusTypeDef PwmInputCapture_Start(TIM_HandleTypeDef *htim3,
                                        TIM_HandleTypeDef *htim4);
void PwmInputCapture_Read(PwmInputCaptureSnapshot snapshots[2]);
void PwmInputCapture_OnInterrupt(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif
