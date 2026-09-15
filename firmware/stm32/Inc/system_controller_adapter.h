#ifndef SYSTEM_CONTROLLER_ADAPTER_H
#define SYSTEM_CONTROLLER_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "capture_snapshot_types.h"

typedef struct
{
  uint16_t pwm1_us;
  uint16_t pwm2_us;
  uint8_t select_corrected;
  uint8_t state;
  uint16_t base_pwm_us;
  uint32_t fault_flags;
  float correction_us;
  float rpm1;
  float rpm2;
  float error_rpm;
  float error_percent;
} SystemControllerAdapterResult;

void SystemControllerAdapter_Init(void);
void SystemControllerAdapter_SetSyncEnabled(uint8_t enabled);
void SystemControllerAdapter_SetManualBypass(uint8_t bypass);
SystemControllerAdapterResult SystemControllerAdapter_Step(
    const HallCaptureSnapshot hall_snapshots[2],
    const PwmInputCaptureSnapshot *base_pwm_snapshot,
    uint32_t now_ms,
    float dt_seconds);

#ifdef __cplusplus
}
#endif

#endif
