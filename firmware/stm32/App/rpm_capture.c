#include "rpm_capture.h"

void rpm_capture_init(rpm_capture_t *capture) {
    if (capture == 0) {
        return;
    }
    capture->previous_tick = 0U;
    capture->period_ticks = 0U;
    capture->last_pulse_ms = 0U;
    capture->has_period = false;
}

void rpm_capture_on_pulse(rpm_capture_t *capture, uint32_t timer_tick, uint32_t now_ms) {
    if (capture == 0) {
        return;
    }
    if (capture->last_pulse_ms != 0U) {
        capture->period_ticks = timer_tick - capture->previous_tick;
        capture->has_period = capture->period_ticks != 0U;
    }
    capture->previous_tick = timer_tick;
    capture->last_pulse_ms = now_ms;
}

bool rpm_capture_calculate(const rpm_capture_t *capture,
                           float timer_hz,
                           float pulses_per_revolution,
                           float *rpm_out) {
    if ((capture == 0) || (rpm_out == 0) || !capture->has_period ||
        (timer_hz <= 0.0F) || (pulses_per_revolution <= 0.0F)) {
        return false;
    }
    *rpm_out = 60.0F * timer_hz /
               ((float)capture->period_ticks * pulses_per_revolution);
    return true;
}
