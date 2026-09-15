#ifndef HALL_CAPTURE_H
#define HALL_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "capture_snapshot_types.h"
#include "stm32g4xx_hal.h"

HAL_StatusTypeDef HallCapture_Start(TIM_HandleTypeDef *htim);
void HallCapture_Read(HallCaptureSnapshot snapshots[2]);
void HallCapture_OnInterrupt(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif
