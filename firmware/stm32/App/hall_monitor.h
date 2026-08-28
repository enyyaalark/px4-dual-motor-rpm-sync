#ifndef HALL_MONITOR_H
#define HALL_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

bool hall_monitor_timed_out(uint32_t now_ms,
                            uint32_t last_pulse_ms,
                            uint32_t timeout_ms);
bool hall_monitor_rpm_plausible(float rpm, float maximum_rpm);

#endif
