#include <cmath>
#include <cstdint>
#include <iostream>

#include "app_types.hpp"
#include "system_controller.hpp"

namespace {

using rpm_sync::AppState;
using rpm_sync::FaultFlag;
using rpm_sync::PwmInputConfig;
using rpm_sync::PwmOutputConfig;
using rpm_sync::RpmValidationConfig;
using rpm_sync::SyncControllerConfig;
using rpm_sync::SystemController;
using rpm_sync::SystemControllerConfig;
using rpm_sync::SystemStepResult;

constexpr RpmValidationConfig kRpmConfig{
    1'000'000.0F,
    2.0F,
    100U,
    20'000.0F,
};

constexpr PwmInputConfig kPwmInputConfig{1'000U, 2'000U, 100U};
constexpr PwmOutputConfig kPwmOutputConfig{1'000U, 2'000U};
constexpr SyncControllerConfig kSyncConfig{
    1.0F,
    0.0F,
    0.0F,
    500.0F,
    100.0F,
    1'000.0F,
};

constexpr SystemControllerConfig kConfig{
    kRpmConfig,
    kPwmInputConfig,
    kPwmOutputConfig,
    kSyncConfig,
    false,
};

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.01F;
}

SystemController configuredController() {
    SystemController controller{};
    controller.config = kConfig;
    rpm_sync::reset(controller);
    rpm_sync::setSelfTestComplete(controller, true);
    rpm_sync::setManualBypass(controller, false);
    rpm_sync::setSyncEnabled(controller, true);
    rpm_sync::onPwmInput(controller, 1'400U, 0U);
    rpm_sync::onHallPulse(controller, 0U, 10'000U, 0U);
    rpm_sync::onHallPulse(controller, 1U, 10'000U, 0U);
    rpm_sync::onHallPulse(controller, 0U, 15'000U, 5U);
    rpm_sync::onHallPulse(controller, 1U, 16'000U, 5U);
    return controller;
}

bool testDefaultConstructionIsInitAndSafe() {
    SystemController controller{};
    const SystemStepResult result = rpm_sync::step(controller, 0U, 0.1F);

    return expect(result.state == AppState::kInit,
                  "a fresh controller must start in INIT") &&
           expect(!result.select_corrected,
                  "a fresh controller must not select corrected PWM") &&
           expect(!result.pwm_output_valid,
                  "unconfigured PWM output must be invalid") &&
           expect(result.pwm1_us == 0U && result.pwm2_us == 0U,
                  "unconfigured PWM channels must be zero") &&
           expect(result.telemetry.correction_us == 0.0F,
                  "unconfigured controller must apply zero correction") &&
           expect(result.telemetry.fault_flags == 0U,
                  "unconfigured controller must not raise runtime faults");
}

bool testMonitorOnlyPassesBaseThrough() {
    SystemController controller = configuredController();
    rpm_sync::setSyncEnabled(controller, false);

    const SystemStepResult result = rpm_sync::step(controller, 5U, 0.1F);
    return expect(result.state == AppState::kMonitorOnly,
                  "disabled sync must remain in MONITOR_ONLY") &&
           expect(result.select_corrected,
                  "ready monitor-only gates must allow corrected selection") &&
           expect(result.pwm_output_valid,
                  "monitor-only PWM output must be valid") &&
           expect(result.pwm1_us == 1'400U && result.pwm2_us == 1'400U,
                  "monitor-only must pass the base PWM through") &&
           expect(result.telemetry.correction_us == 0.0F,
                  "monitor-only must apply zero correction") &&
           expect(near(result.telemetry.error_rpm, 1'000.0F),
                  "monitor-only must still report the RPM error");
}

bool testMissingBaseInputKeepsRawBypassSelected() {
    SystemController controller{};
    controller.config = kConfig;
    rpm_sync::reset(controller);
    rpm_sync::setSelfTestComplete(controller, true);
    rpm_sync::setManualBypass(controller, false);

    const SystemStepResult result = rpm_sync::step(controller, 0U, 0.1F);
    return expect(result.state == AppState::kMonitorOnly,
                  "a waiting base input must remain in MONITOR_ONLY") &&
           expect(!result.select_corrected,
                  "a waiting base input must keep raw PX4 bypass selected") &&
           expect(!result.pwm_output_valid,
                  "a waiting base input must not produce valid outputs") &&
           expect(result.pwm1_us == 0U && result.pwm2_us == 0U,
                  "a waiting base input must leave corrected outputs at zero");
}

bool testInvalidOutputConfigKeepsRawBypassSelected() {
    SystemController controller = configuredController();
    controller.config.pwm_output_config = PwmOutputConfig{};
    rpm_sync::setSyncEnabled(controller, false);

    const SystemStepResult result = rpm_sync::step(controller, 5U, 0.1F);
    return expect(result.state == AppState::kMonitorOnly,
                  "invalid output bounds must remain in MONITOR_ONLY") &&
           expect(!result.select_corrected,
                  "invalid output bounds must keep raw PX4 bypass selected") &&
           expect(!result.pwm_output_valid,
                  "invalid output bounds must reject corrected outputs") &&
           expect(result.pwm1_us == 0U && result.pwm2_us == 0U,
                  "invalid output bounds must leave corrected outputs at zero");
}

bool testSyncControlAppliesBoundedCorrection() {
    SystemController controller = configuredController();
    const SystemStepResult result = rpm_sync::step(controller, 5U, 0.1F);

    return expect(result.state == AppState::kSyncControl,
                  "valid inputs and enable must enter SYNC_CONTROL") &&
           expect(result.select_corrected,
                  "sync control must select corrected PWM") &&
           expect(near(result.telemetry.correction_us, 100.0F),
                  "correction must be clamped by the configured limit") &&
           expect(result.pwm1_us == 1'300U && result.pwm2_us == 1'500U,
                  "correction must subtract from motor 1 and add to motor 2") &&
           expect(result.telemetry.state == AppState::kSyncControl,
                  "telemetry state must match the controller state");
}

bool testManualBypassOverridesReadyConditions() {
    SystemController controller = configuredController();
    rpm_sync::setManualBypass(controller, true);

    const SystemStepResult result = rpm_sync::step(controller, 5U, 0.1F);
    return expect(result.state == AppState::kBypass,
                  "manual bypass must force BYPASS state") &&
           expect(!result.select_corrected,
                  "manual bypass must not select corrected PWM") &&
           expect(result.telemetry.correction_us == 0.0F,
                  "manual bypass must clear correction") &&
           expect(result.pwm1_us == 1'400U && result.pwm2_us == 1'400U,
                  "manual bypass telemetry must pass through the base PWM");
}

bool testHallTimeoutForcesFaultAndRecovers() {
    SystemController controller = configuredController();

    const SystemStepResult timed_out = rpm_sync::step(controller, 200U, 0.1F);
    const std::uint32_t fault_flags = timed_out.telemetry.fault_flags;

    rpm_sync::onHallPulse(controller, 0U, 20'000U, 205U);
    rpm_sync::onHallPulse(controller, 1U, 22'000U, 205U);
    rpm_sync::onPwmInput(controller, 1'400U, 205U);
    const SystemStepResult recovered = rpm_sync::step(controller, 205U, 0.1F);

    return expect(timed_out.state == AppState::kFault,
                  "hall timeout must force FAULT state") &&
           expect(!timed_out.select_corrected,
                  "hall timeout must not select corrected PWM") &&
           expect((fault_flags & rpm_sync::toMask(FaultFlag::kHall1Timeout)) != 0U &&
                      (fault_flags & rpm_sync::toMask(FaultFlag::kHall2Timeout)) != 0U,
                  "hall timeout must set both hall timeout flags") &&
           expect(timed_out.telemetry.correction_us == 0.0F,
                  "fault must clear correction") &&
           expect(recovered.state == AppState::kSyncControl,
                  "a cleared non-latched fault must recover to sync control");
}

bool testPwmTimeoutForcesFaultAndRecovers() {
    SystemController controller = configuredController();
    controller.sync.integral = 42.0F;

    const SystemStepResult timed_out = rpm_sync::step(controller, 101U, 0.1F);
    const std::uint32_t fault_flags = timed_out.telemetry.fault_flags;

    rpm_sync::onPwmInput(controller, 1'400U, 101U);
    const SystemStepResult recovered = rpm_sync::step(controller, 101U, 0.1F);

    return expect(timed_out.state == AppState::kFault,
                  "PWM timeout must force FAULT state") &&
           expect(!timed_out.select_corrected,
                  "PWM timeout must keep raw PX4 bypass selected") &&
           expect((fault_flags &
                   rpm_sync::toMask(FaultFlag::kPwmInputInvalid)) != 0U,
                  "PWM timeout must set the PWM input fault") &&
           expect((fault_flags &
                   (rpm_sync::toMask(FaultFlag::kHall1Timeout) |
                    rpm_sync::toMask(FaultFlag::kHall2Timeout))) == 0U,
                  "isolated PWM timeout must not set hall timeout faults") &&
           expect(timed_out.telemetry.correction_us == 0.0F,
                  "PWM timeout must clear correction") &&
           expect(controller.sync.integral == 0.0F,
                  "PWM timeout must clear controller integral") &&
           expect(recovered.state == AppState::kSyncControl,
                  "a fresh PWM sample must clear the non-latched fault") &&
           expect(recovered.select_corrected,
                  "recovered valid inputs must allow corrected selection") &&
           expect((recovered.telemetry.fault_flags &
                   rpm_sync::toMask(FaultFlag::kPwmInputInvalid)) == 0U,
                  "recovery must clear the active PWM input fault");
}

bool testPwmOutOfRangeForcesFaultAndRecovers() {
    SystemController controller = configuredController();
    controller.sync.integral = 42.0F;
    rpm_sync::onPwmInput(controller, 999U, 6U);

    const SystemStepResult out_of_range =
        rpm_sync::step(controller, 6U, 0.1F);

    rpm_sync::onPwmInput(controller, 1'400U, 6U);
    const SystemStepResult recovered = rpm_sync::step(controller, 6U, 0.1F);

    return expect(out_of_range.state == AppState::kFault,
                  "out-of-range PWM must force FAULT state") &&
           expect(!out_of_range.select_corrected,
                  "out-of-range PWM must keep raw PX4 bypass selected") &&
           expect((out_of_range.telemetry.fault_flags &
                   rpm_sync::toMask(FaultFlag::kPwmInputInvalid)) != 0U,
                  "out-of-range PWM must set the PWM input fault") &&
           expect(out_of_range.telemetry.correction_us == 0.0F,
                  "out-of-range PWM must clear correction") &&
           expect(controller.sync.integral == 0.0F,
                  "out-of-range PWM must clear controller integral") &&
           expect(recovered.state == AppState::kSyncControl,
                  "in-range PWM must clear the non-latched fault") &&
           expect(recovered.select_corrected,
                  "recovered in-range PWM must allow corrected selection");
}

bool testImplausibleHallPulseForcesFaultAndRecovers() {
    SystemController controller = configuredController();
    controller.sync.integral = 42.0F;
    rpm_sync::onHallPulse(controller, 0U, 16'000U, 6U);

    const SystemStepResult implausible =
        rpm_sync::step(controller, 6U, 0.1F);

    rpm_sync::onHallPulse(controller, 0U, 21'000U, 7U);
    const SystemStepResult recovered = rpm_sync::step(controller, 7U, 0.1F);

    return expect(implausible.state == AppState::kFault,
                  "implausible hall pulse must force FAULT state") &&
           expect(!implausible.select_corrected,
                  "implausible hall pulse must keep raw PX4 bypass selected") &&
           expect((implausible.telemetry.fault_flags &
                   rpm_sync::toMask(FaultFlag::kHallImplausible)) != 0U,
                  "implausible hall pulse must set the hall fault") &&
           expect((implausible.telemetry.fault_flags &
                   rpm_sync::toMask(FaultFlag::kPwmInputInvalid)) == 0U,
                  "isolated hall fault must not set the PWM input fault") &&
           expect(implausible.telemetry.correction_us == 0.0F,
                  "implausible hall pulse must clear correction") &&
           expect(controller.sync.integral == 0.0F,
                  "implausible hall pulse must clear controller integral") &&
           expect(recovered.state == AppState::kSyncControl,
                  "a plausible hall period must clear the non-latched fault") &&
           expect(recovered.select_corrected,
                  "recovered hall inputs must allow corrected selection") &&
           expect((recovered.telemetry.fault_flags &
                   rpm_sync::toMask(FaultFlag::kHallImplausible)) == 0U,
                  "recovery must clear the active hall fault");
}

bool testLowRpmDisablesSync() {
    SystemController controller = configuredController();
    rpm_sync::onHallPulse(controller, 0U, 110'000U, 5U);
    rpm_sync::onHallPulse(controller, 1U, 110'000U, 5U);

    const SystemStepResult result = rpm_sync::step(controller, 5U, 0.1F);
    return expect(result.state == AppState::kMonitorOnly,
                  "RPM below the minimum must stay in MONITOR_ONLY") &&
           expect(result.telemetry.correction_us == 0.0F,
                  "low RPM must apply zero correction") &&
           expect(result.pwm1_us == 1'400U && result.pwm2_us == 1'400U,
                  "low RPM must pass the base PWM through");
}

bool testResetClearsRuntimeState() {
    SystemController controller = configuredController();
    (void)rpm_sync::step(controller, 5U, 0.1F);

    rpm_sync::reset(controller);
    const SystemStepResult result = rpm_sync::step(controller, 5U, 0.1F);
    return expect(controller.state == AppState::kInit,
                  "reset must return the controller to INIT") &&
           expect(!controller.bypass.self_test_complete,
                  "reset must clear self-test completion") &&
           expect(controller.bypass.bypass_requested,
                  "reset must request raw PX4 bypass") &&
           expect(!controller.sync_enable_requested,
                  "reset must restore the disabled default sync state") &&
           expect(!result.select_corrected,
                  "reset result must not select corrected PWM");
}

}  // namespace

int main() {
    bool passed = true;
    passed = testDefaultConstructionIsInitAndSafe() && passed;
    passed = testMonitorOnlyPassesBaseThrough() && passed;
    passed = testMissingBaseInputKeepsRawBypassSelected() && passed;
    passed = testInvalidOutputConfigKeepsRawBypassSelected() && passed;
    passed = testSyncControlAppliesBoundedCorrection() && passed;
    passed = testManualBypassOverridesReadyConditions() && passed;
    passed = testHallTimeoutForcesFaultAndRecovers() && passed;
    passed = testPwmTimeoutForcesFaultAndRecovers() && passed;
    passed = testPwmOutOfRangeForcesFaultAndRecovers() && passed;
    passed = testImplausibleHallPulseForcesFaultAndRecovers() && passed;
    passed = testLowRpmDisablesSync() && passed;
    passed = testResetClearsRuntimeState() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "System controller host logic tests passed\n";
    return 0;
}
