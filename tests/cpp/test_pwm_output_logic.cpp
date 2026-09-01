#include <cstdint>
#include <iostream>
#include <limits>

#include "app_config.hpp"
#include "pwm_output.hpp"

namespace {

using rpm_sync::PwmOutputConfig;
using rpm_sync::PwmOutputEvaluation;
using rpm_sync::PwmOutputStatus;

constexpr PwmOutputConfig kSyntheticConfig{1'000U, 2'000U};

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool testBoundariesAndRounding() {
    const PwmOutputEvaluation boundaries =
        rpm_sync::evaluatePwmOutput(1'000.0F, 2'000.0F, kSyntheticConfig);
    const PwmOutputEvaluation rounded =
        rpm_sync::evaluatePwmOutput(1'500.4F, 1'500.6F, kSyntheticConfig);

    return expect(boundaries.status == PwmOutputStatus::kValid,
                  "boundary requests must be valid") &&
           expect(boundaries.output.channel1_us == 1'000U &&
                      boundaries.output.channel2_us == 2'000U,
                  "inclusive boundaries must be preserved") &&
           expect(!boundaries.channel1_limited && !boundaries.channel2_limited,
                  "boundary requests must not be reported as limited") &&
           expect(rounded.output.channel1_us == 1'500U &&
                      rounded.output.channel2_us == 1'501U,
                  "in-range requests must round to the nearest microsecond") &&
           expect(!rounded.channel1_limited && !rounded.channel2_limited,
                  "rounding must not be reported as saturation");
}

bool testIndependentSaturation() {
    const PwmOutputEvaluation both =
        rpm_sync::evaluatePwmOutput(900.0F, 2'100.0F, kSyntheticConfig);
    const PwmOutputEvaluation channel1_only =
        rpm_sync::evaluatePwmOutput(2'001.0F, 1'500.0F, kSyntheticConfig);

    return expect(both.status == PwmOutputStatus::kValid,
                  "finite out-of-range requests must produce bounded outputs") &&
           expect(both.output.channel1_us == 1'000U &&
                      both.output.channel2_us == 2'000U,
                  "both requests must saturate at their nearest boundary") &&
           expect(both.channel1_limited && both.channel2_limited,
                  "both saturated channels must be reported") &&
           expect(channel1_only.output.channel1_us == 2'000U &&
                      channel1_only.output.channel2_us == 1'500U,
                  "one saturated request must not alter the other channel") &&
           expect(channel1_only.channel1_limited &&
                      !channel1_only.channel2_limited,
                  "per-channel saturation flags must remain independent");
}

bool testInvalidConfigurations() {
    const PwmOutputEvaluation reversed = rpm_sync::evaluatePwmOutput(
        1'500.0F, 1'500.0F, PwmOutputConfig{2'000U, 1'000U});
    const PwmOutputEvaluation zero_minimum = rpm_sync::evaluatePwmOutput(
        1'500.0F, 1'500.0F, PwmOutputConfig{0U, 2'000U});

    return expect(reversed.status == PwmOutputStatus::kInvalidConfig &&
                      zero_minimum.status == PwmOutputStatus::kInvalidConfig,
                  "reversed and zero-minimum bounds must be rejected") &&
           expect(reversed.output.channel1_us == 0U &&
                      reversed.output.channel2_us == 0U &&
                      zero_minimum.output.channel1_us == 0U &&
                      zero_minimum.output.channel2_us == 0U,
                  "invalid configurations must not provide usable outputs");
}

bool testInvalidRequestRejectsWholePair() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const PwmOutputEvaluation nan_request =
        rpm_sync::evaluatePwmOutput(nan, 1'500.0F, kSyntheticConfig);
    const PwmOutputEvaluation infinite_request =
        rpm_sync::evaluatePwmOutput(1'500.0F, infinity, kSyntheticConfig);

    return expect(nan_request.status == PwmOutputStatus::kInvalidRequest &&
                      infinite_request.status == PwmOutputStatus::kInvalidRequest,
                  "NaN and infinite requests must be rejected") &&
           expect(nan_request.output.channel1_us == 0U &&
                      nan_request.output.channel2_us == 0U &&
                      infinite_request.output.channel1_us == 0U &&
                      infinite_request.output.channel2_us == 0U,
                  "one invalid request must reject the complete output pair");
}

bool testUncalibratedDefaultsRemainInvalid() {
    const PwmOutputConfig defaults{
        rpm_sync::config::kPwmOutputMinUs,
        rpm_sync::config::kPwmOutputMaxUs,
    };
    return expect(!rpm_sync::pwmOutputConfigValid(defaults),
                  "uncalibrated defaults must not enable PWM output");
}

}  // namespace

int main() {
    bool passed = true;
    passed = testBoundariesAndRounding() && passed;
    passed = testIndependentSaturation() && passed;
    passed = testInvalidConfigurations() && passed;
    passed = testInvalidRequestRejectsWholePair() && passed;
    passed = testUncalibratedDefaultsRemainInvalid() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "PWM output host logic tests passed\n";
    return 0;
}
