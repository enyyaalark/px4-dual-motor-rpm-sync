#include "rpm_capture.hpp"

#include <cmath>

namespace rpm_sync {

void reset(RpmCapture& capture) noexcept {
    capture = {};
}

void onPulse(RpmCapture& capture,
             std::uint32_t timer_tick,
             std::uint32_t now_ms) noexcept {
    if (capture.has_pulse) {
        capture.period_ticks = timer_tick - capture.previous_tick;
        capture.has_period = true;
    }
    capture.previous_tick = timer_tick;
    capture.last_pulse_ms = now_ms;
    capture.has_pulse = true;
}

bool calculateRpm(const RpmCapture& capture,
                  float timer_hz,
                  float pulses_per_revolution,
                  float& rpm_out) noexcept {
    rpm_out = 0.0F;
    if (!capture.has_period || (capture.period_ticks == 0U) ||
        !std::isfinite(timer_hz) || (timer_hz <= 0.0F) ||
        !std::isfinite(pulses_per_revolution) ||
        (pulses_per_revolution <= 0.0F)) {
        return false;
    }
    rpm_out = 60.0F * timer_hz /
              (static_cast<float>(capture.period_ticks) * pulses_per_revolution);
    return true;
}

}  // namespace rpm_sync
