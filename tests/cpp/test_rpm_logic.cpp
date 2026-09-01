#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>

#include "hall_monitor.hpp"
#include "rpm_capture.hpp"

namespace {

using rpm_sync::RpmCapture;
using rpm_sync::RpmReading;
using rpm_sync::RpmStatus;
using rpm_sync::RpmValidationConfig;

constexpr RpmValidationConfig kConfig{
    1'000'000.0F,
    2.0F,
    100U,
    20'000.0F,
};

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool expectNear(float actual, float expected, float tolerance, const char* message) {
    return expect(std::fabs(actual - expected) <= tolerance, message);
}

bool testNormalRpmAndFirstPulseAtZeroMs() {
    RpmCapture capture{};
    rpm_sync::onPulse(capture, 10'000U, 0U);

    const RpmReading first = rpm_sync::evaluateRpm(capture, kConfig, 0U);
    bool ok = expect(first.status == RpmStatus::kWaitingForPeriod,
                     "first pulse must wait for a period") &&
              expect(first.rpm == 0.0F, "first pulse RPM must be zero");

    rpm_sync::onPulse(capture, 15'000U, 5U);
    const RpmReading second = rpm_sync::evaluateRpm(capture, kConfig, 5U);
    ok = expect(second.status == RpmStatus::kValid,
                "second pulse must produce a valid reading") &&
         expect(second.period_ticks == 5'000U, "period must be preserved") &&
         expectNear(second.raw_rpm, 6'000.0F, 0.01F,
                    "raw RPM must match the formula") &&
         expectNear(second.rpm, 6'000.0F, 0.01F,
                    "valid RPM must match raw RPM") &&
         ok;
    return ok;
}

bool testTimerWrap() {
    RpmCapture capture{};
    rpm_sync::onPulse(capture, 0xFFFFFFF0U, 10U);
    rpm_sync::onPulse(capture, 0x00000020U, 11U);

    const RpmValidationConfig config{1'000.0F, 1.0F, 100U, 2'000.0F};
    const RpmReading reading = rpm_sync::evaluateRpm(capture, config, 11U);
    return expect(reading.period_ticks == 48U,
                  "unsigned subtraction must handle one timer wrap") &&
           expect(reading.status == RpmStatus::kValid,
                  "wrapped period must remain valid") &&
           expectNear(reading.rpm, 1'250.0F, 0.01F,
                      "wrapped period must produce the expected RPM");
}

bool testTimeoutAndMillisecondWrap() {
    RpmCapture capture{};
    rpm_sync::onPulse(capture, 100U, 0xFFFFFFF0U);
    rpm_sync::onPulse(capture, 5'100U, 0xFFFFFFF5U);

    const RpmReading at_limit = rpm_sync::evaluateRpm(capture, kConfig, 89U);
    const RpmReading timed_out = rpm_sync::evaluateRpm(capture, kConfig, 90U);
    return expect(at_limit.status == RpmStatus::kValid,
                  "reading at the timeout boundary must remain valid") &&
           expect(timed_out.status == RpmStatus::kTimedOut,
                  "reading after timeout must be marked timed out") &&
           expect(timed_out.rpm == 0.0F,
                  "timed-out effective RPM must be zero") &&
           expect(timed_out.raw_rpm == 0.0F,
                  "timed-out raw RPM must not reuse stale data");
}

bool testInvalidConfiguration() {
    RpmCapture capture{};
    rpm_sync::onPulse(capture, 1'000U, 1U);
    rpm_sync::onPulse(capture, 2'000U, 2U);

    RpmValidationConfig config = kConfig;
    config.pulses_per_revolution = 0.0F;
    const RpmReading zero_ppr = rpm_sync::evaluateRpm(capture, config, 2U);

    config = kConfig;
    config.timer_hz = std::numeric_limits<float>::infinity();
    const RpmReading infinite_timer = rpm_sync::evaluateRpm(capture, config, 2U);

    config = kConfig;
    config.maximum_rpm = std::numeric_limits<float>::quiet_NaN();
    const RpmReading nan_limit = rpm_sync::evaluateRpm(capture, config, 2U);

    return expect(zero_ppr.status == RpmStatus::kInvalidConfig,
                  "zero PPR must be rejected") &&
           expect(zero_ppr.rpm == 0.0F,
                  "invalid configuration must output zero RPM") &&
           expect(infinite_timer.status == RpmStatus::kInvalidConfig,
                  "infinite timer frequency must be rejected") &&
           expect(nan_limit.status == RpmStatus::kInvalidConfig,
                  "NaN maximum RPM must be rejected");
}

bool testImplausiblePulsesAreIsolated() {
    RpmCapture duplicate{};
    rpm_sync::onPulse(duplicate, 1'000U, 1U);
    rpm_sync::onPulse(duplicate, 1'000U, 2U);
    const RpmReading zero_period = rpm_sync::evaluateRpm(duplicate, kConfig, 2U);

    RpmCapture too_fast{};
    rpm_sync::onPulse(too_fast, 1'000U, 1U);
    rpm_sync::onPulse(too_fast, 2'000U, 2U);
    const RpmReading above_limit = rpm_sync::evaluateRpm(too_fast, kConfig, 2U);

    return expect(zero_period.status == RpmStatus::kImplausiblePulse,
                  "zero period must be marked implausible") &&
           expect(zero_period.rpm == 0.0F,
                  "zero-period effective RPM must be zero") &&
           expect(above_limit.status == RpmStatus::kImplausiblePulse,
                  "RPM above the configured limit must be marked implausible") &&
           expectNear(above_limit.raw_rpm, 30'000.0F, 0.01F,
                      "diagnostic raw RPM must be retained") &&
           expect(above_limit.rpm == 0.0F,
                  "implausible raw RPM must not reach the effective RPM");
}

bool testResetClearsState() {
    RpmCapture capture{};
    rpm_sync::onPulse(capture, 100U, 1U);
    rpm_sync::onPulse(capture, 200U, 2U);
    rpm_sync::reset(capture);

    const RpmReading reading = rpm_sync::evaluateRpm(capture, kConfig, 2U);
    return expect(!capture.has_pulse && !capture.has_period,
                  "reset must clear capture history") &&
           expect(reading.status == RpmStatus::kWaitingForPeriod,
                  "reset capture must wait for new pulses") &&
           expect(reading.rpm == 0.0F, "reset RPM must be zero");
}

}  // namespace

int main() {
    bool passed = true;
    passed = testNormalRpmAndFirstPulseAtZeroMs() && passed;
    passed = testTimerWrap() && passed;
    passed = testTimeoutAndMillisecondWrap() && passed;
    passed = testInvalidConfiguration() && passed;
    passed = testImplausiblePulsesAreIsolated() && passed;
    passed = testResetClearsState() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "RPM host logic tests passed\n";
    return 0;
}
