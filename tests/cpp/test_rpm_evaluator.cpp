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

}  // namespace

int main() {
    bool passed = true;
    passed = testValidAndRoundedRpm() && passed;
    passed = testTimeoutZerosRpm() && passed;
    passed = testInvalidPprAndImplausiblePulse() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "RPM C adapter tests passed\n";
    return 0;
}
