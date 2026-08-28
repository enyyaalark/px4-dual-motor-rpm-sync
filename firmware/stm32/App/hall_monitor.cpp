#include "hall_monitor.hpp"

namespace rpm_sync {

bool hallTimedOut(std::uint32_t now_ms,
                  std::uint32_t last_pulse_ms,
                  std::uint32_t timeout_ms) noexcept {
    return (last_pulse_ms == 0U) || (timeout_ms == 0U) ||
           ((now_ms - last_pulse_ms) > timeout_ms);
}

bool rpmPlausible(float rpm, float maximum_rpm) noexcept {
    return (maximum_rpm > 0.0F) && (rpm >= 0.0F) && (rpm <= maximum_rpm);
}

}  // namespace rpm_sync
