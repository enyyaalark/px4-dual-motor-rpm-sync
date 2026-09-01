#pragma once

#include <cstdint>

#include "rpm_capture.hpp"

namespace rpm_sync {

enum class RpmStatus : std::uint8_t {
    kWaitingForPeriod,
    kValid,
    kTimedOut,
    kInvalidConfig,
    kImplausiblePulse,
};

struct RpmValidationConfig {
    float timer_hz{};
    float pulses_per_revolution{};
    std::uint32_t timeout_ms{};
    float maximum_rpm{};
};

struct RpmReading {
    std::uint32_t period_ticks{};
    float raw_rpm{};
    float rpm{};
    RpmStatus status{RpmStatus::kWaitingForPeriod};
};

[[nodiscard]] bool hallTimedOut(bool has_pulse,
                                std::uint32_t now_ms,
                                std::uint32_t last_pulse_ms,
                                std::uint32_t timeout_ms) noexcept;
[[nodiscard]] bool rpmPlausible(float rpm, float maximum_rpm) noexcept;
[[nodiscard]] bool rpmConfigValid(const RpmValidationConfig& config) noexcept;
[[nodiscard]] RpmReading evaluateRpm(const RpmCapture& capture,
                                     const RpmValidationConfig& config,
                                     std::uint32_t now_ms) noexcept;

}  // namespace rpm_sync
