#pragma once

#include <cstdint>

namespace rpm_sync {

struct PwmOutput {
    std::uint16_t channel1_us{};
    std::uint16_t channel2_us{};
};

enum class PwmOutputStatus : std::uint8_t {
    kValid,
    kInvalidConfig,
    kInvalidRequest,
};

struct PwmOutputConfig {
    std::uint16_t minimum_us{};
    std::uint16_t maximum_us{};
};

struct PwmOutputEvaluation {
    PwmOutput output{};
    PwmOutputStatus status{PwmOutputStatus::kInvalidConfig};
    bool channel1_limited{};
    bool channel2_limited{};
};

[[nodiscard]] bool pwmOutputConfigValid(const PwmOutputConfig& config) noexcept;
[[nodiscard]] PwmOutputEvaluation evaluatePwmOutput(
    float channel1_requested_us,
    float channel2_requested_us,
    const PwmOutputConfig& config) noexcept;

}  // namespace rpm_sync
