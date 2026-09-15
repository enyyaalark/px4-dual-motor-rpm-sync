#ifndef CAPTURE_SNAPSHOT_TYPES_H
#define CAPTURE_SNAPSHOT_TYPES_H

#include <stdint.h>

typedef struct
{
  uint32_t period_ticks;
  uint32_t last_pulse_ms;
  uint8_t has_pulse;
  uint8_t has_period;
} HallCaptureSnapshot;

typedef struct
{
  uint32_t period_us;
  uint16_t pulse_width_us;
  uint32_t last_update_ms;
  uint8_t has_sample;
} PwmInputCaptureSnapshot;

#endif
