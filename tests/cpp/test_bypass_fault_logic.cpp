#include <cmath>
#include <iostream>

#include "app_types.hpp"
#include "bypass_control.hpp"
#include "fault_manager.hpp"
#include "sync_controller.hpp"

namespace {

using rpm_sync::BypassControl;
using rpm_sync::FaultFlag;
using rpm_sync::FaultManager;
using rpm_sync::SyncController;
using rpm_sync::SyncControllerConfig;

constexpr SyncControllerConfig kSyntheticControllerConfig{
    1.0F,
    1.0F,
    0.0F,
    100.0F,
    100.0F,
    1'000.0F,
};

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

bool near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.001F;
}

bool testResetAndManualBypassTruthTable() {
    BypassControl control{};

    const bool default_corrected =
        rpm_sync::useCorrectedPwm(control, false);

    control.bypass_requested = false;
    const bool before_self_test =
        rpm_sync::useCorrectedPwm(control, false);

    control.self_test_complete = true;
    const bool ready = rpm_sync::useCorrectedPwm(control, false);

    control.bypass_requested = true;
    const bool manual_bypass =
        rpm_sync::useCorrectedPwm(control, false);

    rpm_sync::reset(control);
    const bool after_reset = rpm_sync::useCorrectedPwm(control, false);

    return expect(control.bypass_requested,
                  "reset must request raw PX4 bypass") &&
           expect(!control.self_test_complete,
                  "reset must clear self-test completion") &&
           expect(!default_corrected,
                  "default construction must not enable corrected PWM") &&
           expect(!before_self_test,
                  "clearing a bypass request before self-test must be ignored") &&
           expect(ready,
                  "corrected PWM may be selected only when all gates are ready") &&
           expect(!manual_bypass,
                  "manual bypass must override a completed self-test") &&
           expect(!after_reset,
                  "reset must return to raw PX4 bypass");
}

bool testActiveFaultClearAndRecovery() {
    FaultManager faults{};
    BypassControl control{};
    control.self_test_complete = true;
    control.bypass_requested = false;

    rpm_sync::setFault(faults, FaultFlag::kHall1Timeout, false);
    const bool during_fault =
        rpm_sync::useCorrectedPwm(control, rpm_sync::hasFault(faults));

    rpm_sync::clearActiveFault(faults, FaultFlag::kHall1Timeout);
    const bool after_clear =
        rpm_sync::useCorrectedPwm(control, rpm_sync::hasFault(faults));

    return expect(!during_fault,
                  "an active non-latched fault must force bypass") &&
           expect(faults.active_flags == 0U && faults.latched_flags == 0U,
                  "clearing a non-latched fault must remove its mask") &&
           expect(after_clear,
                  "a cleared non-latched fault may recover when all gates are ready");
}

bool testLatchedFaultRequiresReset() {
    FaultManager faults{};
    BypassControl control{};
    control.self_test_complete = true;
    control.bypass_requested = false;

    rpm_sync::setFault(faults, FaultFlag::kHall2Timeout, true);
    rpm_sync::clearActiveFault(faults, FaultFlag::kHall2Timeout);
    const bool after_active_clear =
        rpm_sync::useCorrectedPwm(control, rpm_sync::hasFault(faults));

    const bool latch_preserved =
        faults.active_flags == 0U &&
        faults.latched_flags == rpm_sync::toMask(FaultFlag::kHall2Timeout);

    rpm_sync::reset(faults);
    const bool after_fault_reset =
        rpm_sync::useCorrectedPwm(control, rpm_sync::hasFault(faults));

    rpm_sync::reset(control);
    const bool after_full_reset =
        rpm_sync::useCorrectedPwm(control, rpm_sync::hasFault(faults));

    return expect(latch_preserved,
                  "clearing active state must preserve a latched fault") &&
           expect(!after_active_clear,
                  "a latched fault must continue to force bypass") &&
           expect(after_fault_reset,
                  "explicit fault reset may clear the latch when gates stay ready") &&
           expect(!after_full_reset,
                  "full reset must still default the selector to bypass");
}

bool testFaultGateClearsCorrectionAndIntegral() {
    FaultManager faults{};
    BypassControl control{};
    control.self_test_complete = true;
    control.bypass_requested = false;
    SyncController controller{};

    const bool enabled_before_fault =
        rpm_sync::useCorrectedPwm(control, rpm_sync::hasFault(faults));
    const float correction_before_fault = rpm_sync::step(
        controller,
        kSyntheticControllerConfig,
        1'010.0F,
        1'000.0F,
        0.1F,
        enabled_before_fault);
    const float integral_before_fault = controller.integral;

    rpm_sync::setFault(faults, FaultFlag::kHallImplausible, true);
    const bool enabled_during_fault =
        rpm_sync::useCorrectedPwm(control, rpm_sync::hasFault(faults));
    const float correction_during_fault = rpm_sync::step(
        controller,
        kSyntheticControllerConfig,
        1'010.0F,
        1'000.0F,
        0.1F,
        enabled_during_fault);

    return expect(enabled_before_fault,
                  "ready gates must enable the synthetic controller") &&
           expect(correction_before_fault > 0.0F &&
                      integral_before_fault > 0.0F,
                  "test setup must first accumulate a correction") &&
           expect(!enabled_during_fault,
                  "fault manager state must disable corrected PWM") &&
           expect(near(correction_during_fault, 0.0F),
                  "fault gating must return zero correction") &&
           expect(near(controller.integral, 0.0F),
                  "fault gating must clear the controller integral");
}

}  // namespace

int main() {
    bool passed = true;
    passed = testResetAndManualBypassTruthTable() && passed;
    passed = testActiveFaultClearAndRecovery() && passed;
    passed = testLatchedFaultRequiresReset() && passed;
    passed = testFaultGateClearsCorrectionAndIntegral() && passed;
    if (!passed) {
        return 1;
    }
    std::cout << "Bypass and fault host logic tests passed\n";
    return 0;
}
