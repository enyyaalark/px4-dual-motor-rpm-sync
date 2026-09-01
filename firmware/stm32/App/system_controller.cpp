#include "system_controller.hpp"

#include <cmath>

namespace rpm_sync {
namespace {

void refreshTransientFault(FaultManager& faults,
                           FaultFlag flag,
                           bool present) noexcept {
    if (present) {
        setFault(faults, flag, false);
    } else {
        clearActiveFault(faults, flag);
    }
}

bool isHallTimeout(const RpmReading& reading) noexcept {
    return reading.status == RpmStatus::kTimedOut;
}

bool isImplausible(const RpmReading& reading) noexcept {
    return reading.status == RpmStatus::kImplausiblePulse;
}

bool isPwmInputInvalid(const PwmInputReading& reading) noexcept {
    return (reading.status == PwmInputStatus::kTimedOut) ||
           (reading.status == PwmInputStatus::kOutOfRange);
}

float absolute(float value) noexcept {
    return std::fabs(value);
}

}  // namespace

void reset(SystemController& controller) noexcept {
    rpm_sync::reset(controller.rpm_capture[0]);
    rpm_sync::reset(controller.rpm_capture[1]);
    rpm_sync::reset(controller.pwm_input);
    rpm_sync::reset(controller.sync);
    rpm_sync::reset(controller.faults);
    rpm_sync::reset(controller.bypass);
    controller.state = AppState::kInit;
    controller.sync_enable_requested =
        controller.config.sync_control_default_on;
}

void setSelfTestComplete(SystemController& controller, bool complete) noexcept {
    controller.bypass.self_test_complete = complete;
}

void setManualBypass(SystemController& controller, bool requested) noexcept {
    controller.bypass.bypass_requested = requested;
}

void setSyncEnabled(SystemController& controller, bool enabled) noexcept {
    controller.sync_enable_requested = enabled;
}

void onHallPulse(SystemController& controller,
                 std::size_t channel,
                 std::uint32_t timer_tick,
                 std::uint32_t now_ms) noexcept {
    if (channel >= 2U) {
        return;
    }
    rpm_sync::onPulse(controller.rpm_capture[channel], timer_tick, now_ms);
}

void onPwmInput(SystemController& controller,
                std::uint16_t pulse_width_us,
                std::uint32_t now_ms) noexcept {
    rpm_sync::update(controller.pwm_input, pulse_width_us, now_ms);
}

SystemStepResult step(SystemController& controller,
                      std::uint32_t now_ms,
                      float dt_seconds) noexcept {
    const RpmReading rpm1 = evaluateRpm(controller.rpm_capture[0],
                                        controller.config.rpm_config,
                                        now_ms);
    const RpmReading rpm2 = evaluateRpm(controller.rpm_capture[1],
                                        controller.config.rpm_config,
                                        now_ms);
    const PwmInputReading pwm_input = evaluatePwmInput(
        controller.pwm_input,
        controller.config.pwm_input_config,
        now_ms);

    refreshTransientFault(controller.faults,
                          FaultFlag::kHall1Timeout,
                          isHallTimeout(rpm1));
    refreshTransientFault(controller.faults,
                          FaultFlag::kHall2Timeout,
                          isHallTimeout(rpm2));
    refreshTransientFault(controller.faults,
                          FaultFlag::kHallImplausible,
                          isImplausible(rpm1) || isImplausible(rpm2));
    refreshTransientFault(controller.faults,
                          FaultFlag::kPwmInputInvalid,
                          isPwmInputInvalid(pwm_input));

    const bool corrected_allowed =
        useCorrectedPwm(controller.bypass, hasFault(controller.faults));

    const bool rpm_valid = (rpm1.status == RpmStatus::kValid) &&
                           (rpm2.status == RpmStatus::kValid);
    const bool base_valid = pwm_input.status == PwmInputStatus::kValid;
    const bool minimum_rpm_configured =
        std::isfinite(controller.config.sync_config.minimum_rpm) &&
        (controller.config.sync_config.minimum_rpm > 0.0F);
    const bool above_minimum_rpm =
        rpm_valid && minimum_rpm_configured &&
        (rpm1.rpm >= controller.config.sync_config.minimum_rpm) &&
        (rpm2.rpm >= controller.config.sync_config.minimum_rpm);

    const bool sync_enabled = corrected_allowed &&
                              controller.sync_enable_requested &&
                              base_valid && above_minimum_rpm;

    const float error_rpm = rpm_valid ? (rpm1.rpm - rpm2.rpm) : 0.0F;
    const float mean_rpm =
        rpm_valid ? ((rpm1.rpm + rpm2.rpm) * 0.5F) : 0.0F;
    const float error_percent = (rpm_valid && (mean_rpm > 0.0F))
                                    ? (absolute(error_rpm) / mean_rpm * 100.0F)
                                    : 0.0F;

    const float correction_us = rpm_sync::step(controller.sync,
                                               controller.config.sync_config,
                                               rpm1.rpm,
                                               rpm2.rpm,
                                               dt_seconds,
                                               sync_enabled);
    const std::uint16_t base_pwm_us =
        base_valid ? pwm_input.pulse_width_us : 0U;

    SystemStepResult result{};
    result.select_corrected = corrected_allowed;
    result.telemetry.timestamp_ms = now_ms;
    result.telemetry.base_pwm_us = base_pwm_us;
    result.telemetry.rpm1 = rpm1.rpm;
    result.telemetry.rpm2 = rpm2.rpm;
    result.telemetry.error_rpm = error_rpm;
    result.telemetry.error_percent = error_percent;
    result.telemetry.correction_us = correction_us;

    if (base_valid) {
        const PwmOutputEvaluation output = evaluatePwmOutput(
            static_cast<float>(base_pwm_us) - correction_us,
            static_cast<float>(base_pwm_us) + correction_us,
            controller.config.pwm_output_config);
        result.pwm_output_valid = output.status == PwmOutputStatus::kValid;
        result.pwm1_us = output.output.channel1_us;
        result.pwm2_us = output.output.channel2_us;
    } else {
        result.pwm_output_valid = false;
        result.pwm1_us = 0U;
        result.pwm2_us = 0U;
    }

    AppState next_state = AppState::kInit;
    if (!controller.bypass.self_test_complete) {
        next_state = AppState::kInit;
    } else if (hasFault(controller.faults)) {
        next_state = AppState::kFault;
    } else if (controller.bypass.bypass_requested) {
        next_state = AppState::kBypass;
    } else if (sync_enabled) {
        next_state = AppState::kSyncControl;
    } else {
        next_state = AppState::kMonitorOnly;
    }

    controller.state = next_state;
    result.state = next_state;
    result.telemetry.state = next_state;
    result.telemetry.pwm1_us = result.pwm1_us;
    result.telemetry.pwm2_us = result.pwm2_us;
    result.telemetry.fault_flags =
        controller.faults.active_flags | controller.faults.latched_flags;
    return result;
}

}  // namespace rpm_sync
