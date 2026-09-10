#include <cstdint>
#include <iostream>

#include "rpm_evaluator.h"

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool testValidAndRoundedRpm() {
    const RpmEvaluationInput input{5'000U, 5U, 1U, 1U};
    const RpmEvaluationConfig config{1'000'000.0F, 2.0F, 100U, 20'000.0F};
    const RpmEvaluationResult result = RpmEvaluator_Evaluate(&input, &config, 5U);
    return expect(result.status == RPM_EVALUATION_VALID, "valid input status") &&
           expect(result.period_ticks == 5'000U, "period is preserved") &&
           expect(result.raw_rpm == 6'000U, "raw RPM is rounded") &&
           expect(result.rpm == 6'000U, "effective RPM is emitted");
}

bool testTimeoutZerosRpm() {
    const RpmEvaluationInput input{5'000U, 5U, 1U, 1U};
    const RpmEvaluationConfig config{1'000'000.0F, 2.0F, 100U, 20'000.0F};
    const RpmEvaluationResult result = RpmEvaluator_Evaluate(&input, &config, 106U);
    return expect(result.status == RPM_EVALUATION_TIMED_OUT, "timeout status") &&
           expect(result.raw_rpm == 0U, "timeout raw RPM is zero") &&
           expect(result.rpm == 0U, "timeout effective RPM is zero");
}

bool testInvalidPprAndImplausiblePulse() {
    const RpmEvaluationInput input{1'000U, 2U, 1U, 1U};
    const RpmEvaluationConfig invalid{1'000'000.0F, 0.0F, 100U, 20'000.0F};
    const RpmEvaluationConfig bounded{1'000'000.0F, 2.0F, 100U, 20'000.0F};
    const RpmEvaluationResult invalid_result =
        RpmEvaluator_Evaluate(&input, &invalid, 2U);
    const RpmEvaluationResult implausible_result =
        RpmEvaluator_Evaluate(&input, &bounded, 2U);
    return expect(invalid_result.status == RPM_EVALUATION_INVALID_CONFIG,
                  "zero PPR is rejected") &&
           expect(invalid_result.rpm == 0U, "invalid configuration outputs zero") &&
           expect(implausible_result.status == RPM_EVALUATION_IMPLAUSIBLE_PULSE,
                  "over-limit pulse is isolated") &&
           expect(implausible_result.raw_rpm == 30'000U,
                  "implausible raw RPM is retained") &&
           expect(implausible_result.rpm == 0U,
                  "implausible effective RPM is zero");
}

bool testConfiguredMonitorOnlyBounds() {
    const RpmEvaluationInput at_limit{20'000U, 10U, 1U, 1U};
    const RpmEvaluationResult valid_at_timeout =
        RpmEvaluator_EvaluateConfigured(&at_limit, 110U);
    const RpmEvaluationResult timed_out =
        RpmEvaluator_EvaluateConfigured(&at_limit, 111U);

    const RpmEvaluationInput over_limit{18'000U, 10U, 1U, 1U};
    const RpmEvaluationResult implausible =
        RpmEvaluator_EvaluateConfigured(&over_limit, 10U);

    return expect(valid_at_timeout.status == RPM_EVALUATION_VALID,
                  "configured sample is valid at 100 ms boundary") &&
           expect(valid_at_timeout.rpm == 3'000U,
                  "configured PPR converts 20 ms to 3000 RPM") &&
           expect(timed_out.status == RPM_EVALUATION_TIMED_OUT,
                  "configured sample times out after 100 ms") &&
           expect(timed_out.rpm == 0U,
                  "configured timeout zeros effective RPM") &&
           expect(implausible.status == RPM_EVALUATION_IMPLAUSIBLE_PULSE,
                  "configured 3300 RPM limit isolates faster pulse") &&
           expect(implausible.raw_rpm == 3'333U,
                  "configured over-limit raw RPM remains diagnostic") &&
           expect(implausible.rpm == 0U,
                  "configured over-limit effective RPM is zero");
}

}  // namespace

int main() {
    bool passed = true;
    passed = testValidAndRoundedRpm() && passed;
    passed = testTimeoutZerosRpm() && passed;
    passed = testInvalidPprAndImplausiblePulse() && passed;
    passed = testConfiguredMonitorOnlyBounds() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "RPM C adapter tests passed\n";
    return 0;
}
