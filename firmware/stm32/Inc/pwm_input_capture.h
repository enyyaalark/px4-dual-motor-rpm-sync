#ifndef PWM_INPUT_CAPTURE_H
#define PWM_INPUT_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "capture_snapshot_types.h"
#include "stm32g4xx_hal.h"

HAL_StatusTypeDef PwmInputCapture_Start(TIM_HandleTypeDef *htim3,
                                        TIM_HandleTypeDef *htim4);
void PwmInputCapture_Read(PwmInputCaptureSnapshot snapshots[2]);
void PwmInputCapture_OnInterrupt(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif
