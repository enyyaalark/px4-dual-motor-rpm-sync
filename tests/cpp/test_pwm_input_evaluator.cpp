#include "pwm_input_evaluator.h"

#include <cstring>
#include <iostream>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

}  // namespace

int main() {
    const PwmInputEvaluationInput valid{1'300U, 100U, 1U};
    const PwmInputEvaluationResult valid_result =
        PwmInputEvaluator_EvaluateConfigured(&valid, 105U);
    const PwmInputEvaluationInput timed_out{1'300U, 100U, 1U};
    const PwmInputEvaluationResult timed_out_result =
        PwmInputEvaluator_EvaluateConfigured(&timed_out, 111U);
    const PwmInputEvaluationInput out_of_range{900U, 100U, 1U};
    const PwmInputEvaluationResult out_of_range_result =
        PwmInputEvaluator_EvaluateConfigured(&out_of_range, 101U);
    const PwmInputEvaluationResult null_result =
        PwmInputEvaluator_EvaluateConfigured(nullptr, 0U);

    const bool passed =
        expect(valid_result.status == PWM_INPUT_EVALUATION_VALID &&
                   valid_result.raw_pulse_width_us == 1'300U &&
                   valid_result.pulse_width_us == 1'300U,
               "configured adapter must retain a valid width") &&
        expect(timed_out_result.status == PWM_INPUT_EVALUATION_TIMED_OUT &&
                   timed_out_result.raw_pulse_width_us == 1'300U &&
                   timed_out_result.pulse_width_us == 0U,
               "configured adapter must isolate a timed-out width") &&
        expect(out_of_range_result.status == PWM_INPUT_EVALUATION_OUT_OF_RANGE &&
                   out_of_range_result.raw_pulse_width_us == 900U &&
                   out_of_range_result.pulse_width_us == 0U,
               "configured adapter must isolate an out-of-range width") &&
        expect(null_result.status == PWM_INPUT_EVALUATION_INVALID_CONFIG,
               "null adapter input must fail closed") &&
        expect(std::strcmp(PwmInputEvaluator_StatusName(valid_result.status),
                           "VALID") == 0,
               "adapter status name must be stable");
    return passed ? 0 : 1;
}
