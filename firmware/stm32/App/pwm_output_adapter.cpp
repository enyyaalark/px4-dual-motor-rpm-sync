#include "../Inc/pwm_output_adapter.h"

#include "app_config.hpp"
#include "pwm_output.hpp"

namespace rpm_sync {
namespace {

PwmOutputAdapterStatus toCStatus(PwmOutputStatus status) noexcept {
    switch (status) {
        case PwmOutputStatus::kValid:
            return PWM_OUTPUT_ADAPTER_VALID;
        case PwmOutputStatus::kInvalidConfig:
            return PWM_OUTPUT_ADAPTER_INVALID_CONFIG;
        case PwmOutputStatus::kInvalidRequest:
            return PWM_OUTPUT_ADAPTER_INVALID_REQUEST;
    }
    return PWM_OUTPUT_ADAPTER_INVALID_CONFIG;
}

PwmOutputAdapterResult evaluate(const PwmOutputAdapterRequest& request,
                                const PwmOutputAdapterConfig& config) noexcept {
    const PwmOutputConfig cpp_config{config.minimum_us, config.maximum_us};
    const PwmOutputEvaluation evaluation = evaluatePwmOutput(
        request.channel1_requested_us,
        request.channel2_requested_us,
        cpp_config);
    return {
        evaluation.output.channel1_us,
        evaluation.output.channel2_us,
        static_cast<std::uint8_t>(evaluation.channel1_limited),
        static_cast<std::uint8_t>(evaluation.channel2_limited),
        toCStatus(evaluation.status),
    };
}

}  // namespace
}  // namespace rpm_sync

extern "C" PwmOutputAdapterResult PwmOutputAdapter_Evaluate(
    const PwmOutputAdapterRequest* request,
    const PwmOutputAdapterConfig* config) {
    if ((request == nullptr) || (config == nullptr)) {
        return {0U, 0U, 0U, 0U, PWM_OUTPUT_ADAPTER_INVALID_CONFIG};
    }
    return rpm_sync::evaluate(*request, *config);
}

extern "C" PwmOutputAdapterResult PwmOutputAdapter_EvaluateConfigured(
    const PwmOutputAdapterRequest* request) {
    const PwmOutputAdapterConfig config{
        rpm_sync::config::kPwmOutputMinUs,
        rpm_sync::config::kPwmOutputMaxUs,
    };
    return PwmOutputAdapter_Evaluate(request, &config);
}
