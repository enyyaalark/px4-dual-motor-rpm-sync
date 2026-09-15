#include "../Inc/system_controller_adapter.h"

#include "app_config.hpp"
#include "system_controller.hpp"

namespace {

rpm_sync::SystemController controller;
bool initialized = false;

}  // namespace

extern "C" void SystemControllerAdapter_Init(void) {
    using namespace rpm_sync;

    controller.config.rpm_config = {
        config::kHallTimerHz,
        config::kPulsesPerRevolution,
        config::kHallTimeoutMs,
        config::kMaximumRpm,
    };
    controller.config.pwm_input_config = {
        config::kPwmInputMinUs,
        config::kPwmInputMaxUs,
        config::kPwmInputTimeoutMs,
    };
    controller.config.pwm_output_config = {
        config::kPwmOutputMinUs,
        config::kPwmOutputMaxUs,
    };
    controller.config.sync_config = {
        config::kKpDefault,
        config::kKiDefault,
        config::kDeadbandRpmDefault,
        config::kMinClosedLoopRpm,
        config::kCorrectionLimitUs,
        config::kIntegralLimit,
        config::kKdDefault,
        config::kErrorFilterTauSecondsDefault,
    };
    controller.config.base_pwm_mismatch_us = config::kBasePwmMismatchUs;
    controller.config.sync_control_default_on = config::kSyncControlDefaultOn;

    rpm_sync::reset(controller);
    controller.bypass.self_test_complete = true;
    controller.bypass.bypass_requested = false;
    initialized = true;
}

extern "C" void SystemControllerAdapter_SetSyncEnabled(uint8_t enabled) {
    rpm_sync::setSyncEnabled(controller, enabled != 0U);
}

extern "C" void SystemControllerAdapter_SetManualBypass(uint8_t bypass) {
    rpm_sync::setManualBypass(controller, bypass != 0U);
}

extern "C" SystemControllerAdapterResult SystemControllerAdapter_Step(
    const HallCaptureSnapshot hall_snapshots[2],
    const PwmInputCaptureSnapshot base_pwm_snapshots[2],
    uint32_t now_ms,
    float dt_seconds) {
    if (!initialized) {
        SystemControllerAdapter_Init();
    }

    // The bench rig has Hall1 (TIM2_CH1) physically on the motor driven by
    // TIM1_CH3 and Hall2 (TIM2_CH2) on the motor driven by TIM1_CH1. The
    // control model expects rpm1 to be the TIM1_CH1 motor and rpm2 to be the
    // TIM1_CH3 motor, so swap the Hall sources before feeding the controller.
    for (uint32_t channel = 0U; channel < 2U; ++channel) {
        const uint32_t hall_channel = 1U - channel;
        controller.rpm_capture[channel].period_ticks =
            hall_snapshots[hall_channel].period_ticks;
        controller.rpm_capture[channel].last_pulse_ms =
            hall_snapshots[hall_channel].last_pulse_ms;
        controller.rpm_capture[channel].has_pulse =
            hall_snapshots[hall_channel].has_pulse != 0U;
        controller.rpm_capture[channel].has_period =
            hall_snapshots[hall_channel].has_period != 0U;
    }

    if (base_pwm_snapshots != nullptr) {
        for (uint32_t channel = 0U; channel < 2U; ++channel) {
            controller.pwm_input[channel].pulse_width_us =
                base_pwm_snapshots[channel].pulse_width_us;
            controller.pwm_input[channel].last_update_ms =
                base_pwm_snapshots[channel].last_update_ms;
            controller.pwm_input[channel].has_sample =
                base_pwm_snapshots[channel].has_sample != 0U;
        }
    }

    const rpm_sync::SystemStepResult result =
        rpm_sync::step(controller, now_ms, dt_seconds);

    SystemControllerAdapterResult out{};
    out.pwm1_us = result.pwm1_us;
    out.pwm2_us = result.pwm2_us;
    out.select_corrected = result.select_corrected ? 1U : 0U;
    out.state = static_cast<uint8_t>(result.state);
    out.base_pwm_us = result.telemetry.base_pwm_us;
    out.fault_flags = result.telemetry.fault_flags;
    out.correction_us = result.telemetry.correction_us;
    out.rpm1 = result.telemetry.rpm1;
    out.rpm2 = result.telemetry.rpm2;
    out.error_rpm = result.telemetry.error_rpm;
    out.error_percent = result.telemetry.error_percent;
    return out;
}
