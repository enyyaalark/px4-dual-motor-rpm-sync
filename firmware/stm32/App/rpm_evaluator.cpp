#include "rpm_evaluator.h"

#include <cmath>
#include <cstdint>
#include <limits>

#include "app_config.hpp"
#include "hall_monitor.hpp"

namespace rpm_sync {
namespace {

RpmEvaluationStatus toCStatus(RpmStatus status) noexcept {
    switch (status) {
        case RpmStatus::kWaitingForPeriod:
            return RPM_EVALUATION_WAITING_FOR_PERIOD;
        case RpmStatus::kValid:
            return RPM_EVALUATION_VALID;
        case RpmStatus::kTimedOut:
            return RPM_EVALUATION_TIMED_OUT;
        case RpmStatus::kInvalidConfig:
            return RPM_EVALUATION_INVALID_CONFIG;
        case RpmStatus::kImplausiblePulse:
            return RPM_EVALUATION_IMPLAUSIBLE_PULSE;
    }
    return RPM_EVALUATION_INVALID_CONFIG;
}

std::uint32_t roundedRpm(float rpm) noexcept {
    if (!std::isfinite(rpm) || (rpm <= 0.0F)) {
        return 0U;
    }
    constexpr float kMaximumUint32 =
        static_cast<float>(std::numeric_limits<std::uint32_t>::max());
    if (rpm >= kMaximumUint32) {
        return std::numeric_limits<std::uint32_t>::max();
    }
    return static_cast<std::uint32_t>(rpm + 0.5F);
}

RpmEvaluationResult evaluate(const RpmEvaluationInput& input,
                             const RpmEvaluationConfig& config,
                             std::uint32_t now_ms) noexcept {
    RpmCapture capture{};
    capture.period_ticks = input.period_ticks;
    capture.last_pulse_ms = input.last_pulse_ms;
    capture.has_pulse = input.has_pulse != 0U;
    capture.has_period = input.has_period != 0U;

    const RpmValidationConfig validation_config{
        config.timer_hz,
        config.pulses_per_revolution,
        config.timeout_ms,
        config.maximum_rpm,
    };
    const RpmReading reading = evaluateRpm(capture, validation_config, now_ms);
    return {
        reading.period_ticks,
        roundedRpm(reading.raw_rpm),
        roundedRpm(reading.rpm),
        toCStatus(reading.status),
    };
}

}  // namespace
}  // namespace rpm_sync

extern "C" RpmEvaluationResult RpmEvaluator_Evaluate(
    const RpmEvaluationInput* input,
    const RpmEvaluationConfig* config,
    std::uint32_t now_ms) {
    if ((input == nullptr) || (config == nullptr)) {
        return {0U, 0U, 0U, RPM_EVALUATION_INVALID_CONFIG};
    }
    return rpm_sync::evaluate(*input, *config, now_ms);
}

extern "C" RpmEvaluationResult RpmEvaluator_EvaluateConfigured(
    const RpmEvaluationInput* input,
    std::uint32_t now_ms) {
    const RpmEvaluationConfig config{
        rpm_sync::config::kHallTimerHz,
        rpm_sync::config::kPulsesPerRevolution,
        rpm_sync::config::kHallTimeoutMs,
        rpm_sync::config::kMaximumRpm,
    };
    return RpmEvaluator_Evaluate(input, &config, now_ms);
}

extern "C" const char* RpmEvaluator_StatusName(RpmEvaluationStatus status) {
    switch (status) {
        case RPM_EVALUATION_WAITING_FOR_PERIOD:
            return "WAITING";
        case RPM_EVALUATION_VALID:
            return "VALID";
        case RPM_EVALUATION_TIMED_OUT:
            return "TIMED_OUT";
        case RPM_EVALUATION_INVALID_CONFIG:
            return "INVALID_CONFIG";
        case RPM_EVALUATION_IMPLAUSIBLE_PULSE:
            return "IMPLAUSIBLE_PULSE";
    }
    return "INVALID_CONFIG";
}
