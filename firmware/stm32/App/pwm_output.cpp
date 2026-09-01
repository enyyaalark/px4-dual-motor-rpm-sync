#include "pwm_output.hpp"

#include <cmath>

namespace rpm_sync {
namespace {

std::uint16_t limitAndRound(float requested_us,
                            const PwmOutputConfig& config) noexcept {
    if (requested_us <= static_cast<float>(config.minimum_us)) {
        return config.minimum_us;
    }
    if (requested_us >= static_cast<float>(config.maximum_us)) {
        return config.maximum_us;
    }
    return static_cast<std::uint16_t>(requested_us + 0.5F);
}

}  // namespace

bool pwmOutputConfigValid(const PwmOutputConfig& config) noexcept {
    return (config.minimum_us > 0U) &&
           (config.minimum_us <= config.maximum_us);
}

PwmOutputEvaluation evaluatePwmOutput(
    float channel1_requested_us,
    float channel2_requested_us,
    const PwmOutputConfig& config) noexcept {
    PwmOutputEvaluation evaluation{};

    if (!pwmOutputConfigValid(config)) {
        return evaluation;
    }

    if (!std::isfinite(channel1_requested_us) ||
        !std::isfinite(channel2_requested_us)) {
        evaluation.status = PwmOutputStatus::kInvalidRequest;
        return evaluation;
    }

    evaluation.output.channel1_us =
        limitAndRound(channel1_requested_us, config);
    evaluation.output.channel2_us =
        limitAndRound(channel2_requested_us, config);
    evaluation.channel1_limited =
        (channel1_requested_us < static_cast<float>(config.minimum_us)) ||
        (channel1_requested_us > static_cast<float>(config.maximum_us));
    evaluation.channel2_limited =
        (channel2_requested_us < static_cast<float>(config.minimum_us)) ||
        (channel2_requested_us > static_cast<float>(config.maximum_us));
    evaluation.status = PwmOutputStatus::kValid;
    return evaluation;
}

}  // namespace rpm_sync
