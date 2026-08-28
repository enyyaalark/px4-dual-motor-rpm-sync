#ifndef RPM_CAPTURE_H
#define RPM_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t previous_tick;
    uint32_t period_ticks;
    uint32_t last_pulse_ms;
    bool has_period;
} rpm_capture_t;

void rpm_capture_init(rpm_capture_t *capture);
void rpm_capture_on_pulse(rpm_capture_t *capture, uint32_t timer_tick, uint32_t now_ms);
bool rpm_capture_calculate(const rpm_capture_t *capture,
                           float timer_hz,
                           float pulses_per_revolution,
                           float *rpm_out);

#endif
