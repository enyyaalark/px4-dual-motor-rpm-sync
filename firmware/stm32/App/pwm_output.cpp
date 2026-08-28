#include "pwm_output.hpp"

namespace rpm_sync {

std::uint16_t clampPwm(float requested_us,
                       std::uint16_t minimum_us,
                       std::uint16_t maximum_us) noexcept {
    if (requested_us <= static_cast<float>(minimum_us)) {
        return minimum_us;
    }
    if (requested_us >= static_cast<float>(maximum_us)) {
        return maximum_us;
    }
    return static_cast<std::uint16_t>(requested_us + 0.5F);
}

void set(PwmOutput& output,
         std::uint16_t channel1_us,
         std::uint16_t channel2_us) noexcept {
    output.channel1_us = channel1_us;
    output.channel2_us = channel2_us;
    // Board-specific HAL timer writes belong in the future hardware adapter.
}

}  // namespace rpm_sync
