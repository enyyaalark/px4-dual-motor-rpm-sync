#include "pwm_input.hpp"

#include "app_config.hpp"

namespace rpm_sync {

void reset(PwmInput& input) noexcept {
    input = {};
}

void update(PwmInput& input,
            std::uint16_t pulse_width_us,
            std::uint32_t now_ms) noexcept {
    input.pulse_width_us = pulse_width_us;
    input.last_update_ms = now_ms;
    input.valid = (pulse_width_us >= config::kPwmMinUs) &&
                  (pulse_width_us <= config::kPwmMaxUs);
}

bool isFresh(const PwmInput& input,
             std::uint32_t now_ms,
             std::uint32_t timeout_ms) noexcept {
    return input.valid && (timeout_ms > 0U) &&
           ((now_ms - input.last_update_ms) <= timeout_ms);
}

}  // namespace rpm_sync
