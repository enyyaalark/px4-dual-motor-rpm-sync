#pragma once

#include <cstdint>

namespace rpm_sync {

struct PwmInput {
    std::uint16_t pulse_width_us{};
    std::uint32_t last_update_ms{};
    bool has_sample{};
};

enum class PwmInputStatus : std::uint8_t {
    kWaitingForSample,
    kValid,
    kTimedOut,
    kOutOfRange,
    kInvalidConfig,
};

struct PwmInputConfig {
    std::uint16_t minimum_us{};
    std::uint16_t maximum_us{};
    std::uint32_t timeout_ms{};
};

struct PwmInputReading {
    std::uint16_t raw_pulse_width_us{};
    std::uint16_t pulse_width_us{};
    PwmInputStatus status{PwmInputStatus::kWaitingForSample};
};

void reset(PwmInput& input) noexcept;
void update(PwmInput& input, std::uint16_t pulse_width_us, std::uint32_t now_ms) noexcept;
[[nodiscard]] bool isFresh(const PwmInput& input,
                           std::uint32_t now_ms,
                           std::uint32_t timeout_ms) noexcept;
[[nodiscard]] bool pwmInputConfigValid(const PwmInputConfig& config) noexcept;
[[nodiscard]] PwmInputReading evaluatePwmInput(const PwmInput& input,
                                               const PwmInputConfig& config,
                                               std::uint32_t now_ms) noexcept;

}  // namespace rpm_sync
