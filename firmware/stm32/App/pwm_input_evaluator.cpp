#include "../Inc/pwm_input_evaluator.h"

#include <cstdint>

#include "app_config.hpp"
#include "pwm_input.hpp"

namespace rpm_sync {
namespace {

PwmInputEvaluationStatus toCStatus(PwmInputStatus status) noexcept {
    switch (status) {
        case PwmInputStatus::kWaitingForSample:
            return PWM_INPUT_EVALUATION_WAITING;
        case PwmInputStatus::kValid:
            return PWM_INPUT_EVALUATION_VALID;
        case PwmInputStatus::kTimedOut:
            return PWM_INPUT_EVALUATION_TIMED_OUT;
        case PwmInputStatus::kOutOfRange:
            return PWM_INPUT_EVALUATION_OUT_OF_RANGE;
        case PwmInputStatus::kInvalidConfig:
            return PWM_INPUT_EVALUATION_INVALID_CONFIG;
    }
    return PWM_INPUT_EVALUATION_INVALID_CONFIG;
}

}  // namespace
}  // namespace rpm_sync

extern "C" PwmInputEvaluationResult PwmInputEvaluator_EvaluateConfigured(
    const PwmInputEvaluationInput* input,
    std::uint32_t now_ms) {
    if (input == nullptr) {
        return {0U, 0U, PWM_INPUT_EVALUATION_INVALID_CONFIG};
    }

    const rpm_sync::PwmInput sample{
        input->pulse_width_us,
        input->last_update_ms,
        input->has_sample != 0U,
    };
    const rpm_sync::PwmInputConfig config{
        rpm_sync::config::kPwmInputMinUs,
        rpm_sync::config::kPwmInputMaxUs,
        rpm_sync::config::kPwmInputTimeoutMs,
    };
    const rpm_sync::PwmInputReading reading =
        rpm_sync::evaluatePwmInput(sample, config, now_ms);
    return {
        reading.raw_pulse_width_us,
        reading.pulse_width_us,
        rpm_sync::toCStatus(reading.status),
    };
}

extern "C" const char* PwmInputEvaluator_StatusName(
    PwmInputEvaluationStatus status) {
    switch (status) {
        case PWM_INPUT_EVALUATION_WAITING:
            return "WAITING";
        case PWM_INPUT_EVALUATION_VALID:
            return "VALID";
        case PWM_INPUT_EVALUATION_TIMED_OUT:
            return "TIMED_OUT";
        case PWM_INPUT_EVALUATION_OUT_OF_RANGE:
            return "OUT_OF_RANGE";
        case PWM_INPUT_EVALUATION_INVALID_CONFIG:
            return "INVALID_CONFIG";
    }
    return "INVALID_CONFIG";
}
