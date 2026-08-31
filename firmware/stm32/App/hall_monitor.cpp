#include "hall_monitor.hpp"

#include <cmath>

namespace rpm_sync {

bool hallTimedOut(bool has_pulse,
                  std::uint32_t now_ms,
                  std::uint32_t last_pulse_ms,
                  std::uint32_t timeout_ms) noexcept {
    return !has_pulse || (timeout_ms == 0U) ||
           ((now_ms - last_pulse_ms) > timeout_ms);
}

bool rpmPlausible(float rpm, float maximum_rpm) noexcept {
    return std::isfinite(rpm) && std::isfinite(maximum_rpm) &&
           (maximum_rpm > 0.0F) && (rpm >= 0.0F) && (rpm <= maximum_rpm);
}

bool rpmConfigValid(const RpmValidationConfig& config) noexcept {
    return std::isfinite(config.timer_hz) && (config.timer_hz > 0.0F) &&
           std::isfinite(config.pulses_per_revolution) &&
           (config.pulses_per_revolution > 0.0F) &&
           (config.timeout_ms > 0U) && std::isfinite(config.maximum_rpm) &&
           (config.maximum_rpm > 0.0F);
}

RpmReading evaluateRpm(const RpmCapture& capture,
                       const RpmValidationConfig& config,
                       std::uint32_t now_ms) noexcept {
    RpmReading reading{};
    reading.period_ticks = capture.period_ticks;

    if (!rpmConfigValid(config)) {
        reading.status = RpmStatus::kInvalidConfig;
        return reading;
    }

    if (!capture.has_pulse) {
        return reading;
    }

    if (hallTimedOut(capture.has_pulse,
                     now_ms,
                     capture.last_pulse_ms,
                     config.timeout_ms)) {
        reading.status = RpmStatus::kTimedOut;
        return reading;
    }

    if (!capture.has_period) {
        return reading;
    }

    if (!calculateRpm(capture,
                      config.timer_hz,
                      config.pulses_per_revolution,
                      reading.raw_rpm)) {
        reading.status = RpmStatus::kImplausiblePulse;
        return reading;
    }

    if (!rpmPlausible(reading.raw_rpm, config.maximum_rpm)) {
        reading.status = RpmStatus::kImplausiblePulse;
        return reading;
    }

    reading.rpm = reading.raw_rpm;
    reading.status = RpmStatus::kValid;
    return reading;
}

}  // namespace rpm_sync
