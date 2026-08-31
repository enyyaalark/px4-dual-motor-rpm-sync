#include <cstdint>
#include <iostream>

#include "app_config.hpp"
#include "pwm_input.hpp"

namespace {

using rpm_sync::PwmInput;
using rpm_sync::PwmInputConfig;
using rpm_sync::PwmInputReading;
using rpm_sync::PwmInputStatus;

constexpr PwmInputConfig kSyntheticConfig{1'000U, 2'000U, 100U};

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool testWaitingAndZeroMillisecondSample() {
    PwmInput input{};
    const PwmInputReading waiting =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 0U);

    rpm_sync::update(input, 1'500U, 0U);
    const PwmInputReading valid =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 0U);

    return expect(waiting.status == PwmInputStatus::kWaitingForSample,
                  "reset input must wait for its first sample") &&
           expect(waiting.pulse_width_us == 0U,
                  "waiting effective pulse width must be zero") &&
           expect(valid.status == PwmInputStatus::kValid,
                  "a sample at zero milliseconds must be valid") &&
           expect(valid.raw_pulse_width_us == 1'500U,
                  "raw pulse width must be retained") &&
           expect(valid.pulse_width_us == 1'500U,
                  "valid effective pulse width must match the sample");
}

bool testInclusiveBoundaries() {
    PwmInput input{};
    rpm_sync::update(input, kSyntheticConfig.minimum_us, 1U);
    const PwmInputReading minimum =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 1U);

    rpm_sync::update(input, kSyntheticConfig.maximum_us, 2U);
    const PwmInputReading maximum =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 2U);

    return expect(minimum.status == PwmInputStatus::kValid,
                  "minimum boundary must be valid") &&
           expect(minimum.pulse_width_us == kSyntheticConfig.minimum_us,
                  "minimum boundary must be preserved") &&
           expect(maximum.status == PwmInputStatus::kValid,
                  "maximum boundary must be valid") &&
           expect(maximum.pulse_width_us == kSyntheticConfig.maximum_us,
                  "maximum boundary must be preserved");
}

bool testOutOfRangeIsIsolated() {
    PwmInput input{};
    rpm_sync::update(input, 999U, 1U);
    const PwmInputReading below =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 1U);

    rpm_sync::update(input, 2'001U, 2U);
    const PwmInputReading above =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 2U);

    return expect(below.status == PwmInputStatus::kOutOfRange,
                  "sample below the configured range must be rejected") &&
           expect(below.raw_pulse_width_us == 999U,
                  "below-range raw sample must remain diagnosable") &&
           expect(below.pulse_width_us == 0U,
                  "below-range effective pulse width must be zero") &&
           expect(above.status == PwmInputStatus::kOutOfRange,
                  "sample above the configured range must be rejected") &&
           expect(above.raw_pulse_width_us == 2'001U,
                  "above-range raw sample must remain diagnosable") &&
           expect(above.pulse_width_us == 0U,
                  "above-range effective pulse width must be zero");
}

bool testTimeoutAcrossMillisecondWrap() {
    PwmInput input{};
    rpm_sync::update(input, 1'500U, 0xFFFFFFF0U);

    const PwmInputReading at_limit =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 84U);
    const PwmInputReading timed_out =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 85U);

    return expect(at_limit.status == PwmInputStatus::kValid,
                  "sample at the timeout boundary must remain valid") &&
           expect(timed_out.status == PwmInputStatus::kTimedOut,
                  "sample after timeout must be marked timed out") &&
           expect(timed_out.raw_pulse_width_us == 1'500U,
                  "timed-out raw sample must remain diagnosable") &&
           expect(timed_out.pulse_width_us == 0U,
                  "timed-out effective pulse width must be zero");
}

bool testInvalidConfigurations() {
    PwmInput input{};
    rpm_sync::update(input, 1'500U, 1U);

    const PwmInputReading reversed = rpm_sync::evaluatePwmInput(
        input, PwmInputConfig{2'000U, 1'000U, 100U}, 1U);
    const PwmInputReading zero_minimum = rpm_sync::evaluatePwmInput(
        input, PwmInputConfig{0U, 2'000U, 100U}, 1U);
    const PwmInputReading zero_timeout = rpm_sync::evaluatePwmInput(
        input, PwmInputConfig{1'000U, 2'000U, 0U}, 1U);

    return expect(reversed.status == PwmInputStatus::kInvalidConfig,
                  "reversed limits must be rejected") &&
           expect(zero_minimum.status == PwmInputStatus::kInvalidConfig,
                  "zero minimum must be rejected") &&
           expect(zero_timeout.status == PwmInputStatus::kInvalidConfig,
                  "zero timeout must be rejected") &&
           expect(reversed.pulse_width_us == 0U &&
                      zero_minimum.pulse_width_us == 0U &&
                      zero_timeout.pulse_width_us == 0U,
                  "invalid configurations must output zero effective width");
}

bool testUncalibratedDefaultsRemainInvalid() {
    const PwmInputConfig defaults{
        rpm_sync::config::kPwmInputMinUs,
        rpm_sync::config::kPwmInputMaxUs,
        rpm_sync::config::kPwmInputTimeoutMs,
    };
    return expect(!rpm_sync::pwmInputConfigValid(defaults),
                  "uncalibrated project defaults must not enable PWM input");
}

bool testResetClearsSample() {
    PwmInput input{};
    rpm_sync::update(input, 1'500U, 10U);
    rpm_sync::reset(input);
    const PwmInputReading reading =
        rpm_sync::evaluatePwmInput(input, kSyntheticConfig, 10U);

    return expect(!input.has_sample, "reset must clear sample presence") &&
           expect(input.pulse_width_us == 0U,
                  "reset must clear the stored raw pulse width") &&
           expect(reading.status == PwmInputStatus::kWaitingForSample,
                  "reset input must wait for a new sample") &&
           expect(reading.pulse_width_us == 0U,
                  "reset effective pulse width must be zero");
}

}  // namespace

int main() {
    bool passed = true;
    passed = testWaitingAndZeroMillisecondSample() && passed;
    passed = testInclusiveBoundaries() && passed;
    passed = testOutOfRangeIsIsolated() && passed;
    passed = testTimeoutAcrossMillisecondWrap() && passed;
    passed = testInvalidConfigurations() && passed;
    passed = testUncalibratedDefaultsRemainInvalid() && passed;
    passed = testResetClearsSample() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "PWM input host logic tests passed\n";
    return 0;
}
