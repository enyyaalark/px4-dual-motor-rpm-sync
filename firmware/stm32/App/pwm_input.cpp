#include "pwm_input.hpp"

namespace rpm_sync {

void reset(PwmInput& input) noexcept {
    input = {};
}

void update(PwmInput& input,
            std::uint16_t pulse_width_us,
            std::uint32_t now_ms) noexcept {
    input.pulse_width_us = pulse_width_us;
    input.last_update_ms = now_ms;
    input.has_sample = true;
}

bool isFresh(const PwmInput& input,
             std::uint32_t now_ms,
             std::uint32_t timeout_ms) noexcept {
    return input.has_sample && (timeout_ms > 0U) &&
           ((now_ms - input.last_update_ms) <= timeout_ms);
}

bool pwmInputConfigValid(const PwmInputConfig& config) noexcept {
    return (config.minimum_us > 0U) &&
           (config.minimum_us <= config.maximum_us) &&
           (config.timeout_ms > 0U);
}

PwmInputReading evaluatePwmInput(const PwmInput& input,
                                 const PwmInputConfig& config,
                                 std::uint32_t now_ms) noexcept {
    PwmInputReading reading{};
    reading.raw_pulse_width_us = input.pulse_width_us;

    if (!pwmInputConfigValid(config)) {
        reading.status = PwmInputStatus::kInvalidConfig;
        return reading;
    }

    if (!input.has_sample) {
        return reading;
    }

    if (!isFresh(input, now_ms, config.timeout_ms)) {
        reading.status = PwmInputStatus::kTimedOut;
        return reading;
    }

    if ((input.pulse_width_us < config.minimum_us) ||
        (input.pulse_width_us > config.maximum_us)) {
        reading.status = PwmInputStatus::kOutOfRange;
        return reading;
    }

    reading.pulse_width_us = input.pulse_width_us;
    reading.status = PwmInputStatus::kValid;
    return reading;
}

}  // namespace rpm_sync
