#include <cmath>
#include <iostream>

#include "pwm_output_adapter.h"

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool testValidRequestIsRoundedAndLimitedAsOnePair() {
    const PwmOutputAdapterRequest request{999.0F, 1500.6F};
    const PwmOutputAdapterConfig config{1000U, 2000U};
    const PwmOutputAdapterResult result =
        PwmOutputAdapter_Evaluate(&request, &config);
    return expect(result.status == PWM_OUTPUT_ADAPTER_VALID, "valid status") &&
           expect(result.channel1_us == 1000U, "channel 1 is limited") &&
           expect(result.channel2_us == 1501U, "channel 2 is rounded") &&
           expect(result.channel1_limited == 1U, "limit flag is set") &&
           expect(result.channel2_limited == 0U, "rounding is not limiting");
}

bool testInvalidRequestReturnsNoPartialPair() {
    const PwmOutputAdapterRequest request{1500.0F, NAN};
    const PwmOutputAdapterConfig config{1000U, 2000U};
    const PwmOutputAdapterResult result =
        PwmOutputAdapter_Evaluate(&request, &config);
    return expect(result.status == PWM_OUTPUT_ADAPTER_INVALID_REQUEST,
                  "invalid request status") &&
           expect(result.channel1_us == 0U && result.channel2_us == 0U,
                  "invalid pair stays zero");
}

bool testConfiguredDefaultsKeepTargetOutputDisabled() {
    const PwmOutputAdapterRequest request{1500.0F, 1500.0F};
    const PwmOutputAdapterResult result =
        PwmOutputAdapter_EvaluateConfigured(&request);
    return expect(result.status == PWM_OUTPUT_ADAPTER_INVALID_CONFIG,
                  "TBD bounds keep output invalid") &&
           expect(result.channel1_us == 0U && result.channel2_us == 0U,
                  "invalid configured output stays zero");
}

bool testNullPointersAreRejected() {
    const PwmOutputAdapterRequest request{1500.0F, 1500.0F};
    const PwmOutputAdapterConfig config{1000U, 2000U};
    return expect(PwmOutputAdapter_Evaluate(nullptr, &config).status ==
                      PWM_OUTPUT_ADAPTER_INVALID_CONFIG,
                  "null request is rejected") &&
           expect(PwmOutputAdapter_Evaluate(&request, nullptr).status ==
                      PWM_OUTPUT_ADAPTER_INVALID_CONFIG,
                  "null config is rejected");
}

}  // namespace

int main() {
    bool passed = true;
    passed = testValidRequestIsRoundedAndLimitedAsOnePair() && passed;
    passed = testInvalidRequestReturnsNoPartialPair() && passed;
    passed = testConfiguredDefaultsKeepTargetOutputDisabled() && passed;
    passed = testNullPointersAreRejected() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "PWM output C adapter tests passed\n";
    return 0;
}
