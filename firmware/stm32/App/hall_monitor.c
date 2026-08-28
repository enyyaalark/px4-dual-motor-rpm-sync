#include "hall_monitor.h"

bool hall_monitor_timed_out(uint32_t now_ms,
                            uint32_t last_pulse_ms,
                            uint32_t timeout_ms) {
    return (last_pulse_ms == 0U) || (timeout_ms == 0U) ||
           ((uint32_t)(now_ms - last_pulse_ms) > timeout_ms);
}

bool hall_monitor_rpm_plausible(float rpm, float maximum_rpm) {
    return (maximum_rpm > 0.0F) && (rpm >= 0.0F) && (rpm <= maximum_rpm);
}
