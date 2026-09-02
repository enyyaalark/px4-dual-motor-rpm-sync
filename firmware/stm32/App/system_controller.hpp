#pragma once

#include <cstddef>
#include <cstdint>

#include "app_types.hpp"
#include "bypass_control.hpp"
#include "fault_manager.hpp"
#include "hall_monitor.hpp"
#include "pwm_input.hpp"
#include "pwm_output.hpp"
#include "rpm_capture.hpp"
#include "sync_controller.hpp"

namespace rpm_sync {

struct SystemControllerConfig {
    RpmValidationConfig rpm_config{};
    PwmInputConfig pwm_input_config{};
    PwmOutputConfig pwm_output_config{};
    SyncControllerConfig sync_config{};
    bool sync_control_default_on{};
};

struct SystemController {
    SystemControllerConfig config{};
    RpmCapture rpm_capture[2]{};
    PwmInput pwm_input{};
    SyncController sync{};
    FaultManager faults{};
    BypassControl bypass{};
    AppState state{AppState::kInit};
    bool sync_enable_requested{};
};

struct SystemStepResult {
    AppState state{AppState::kInit};
    bool select_corrected{};
    bool pwm_output_valid{};
    std::uint16_t pwm1_us{};
    std::uint16_t pwm2_us{};
    TelemetrySample telemetry{};
};

void reset(SystemController& controller) noexcept;
void setSelfTestComplete(SystemController& controller, bool complete) noexcept;
void setManualBypass(SystemController& controller, bool requested) noexcept;
void setSyncEnabled(SystemController& controller, bool enabled) noexcept;

void onHallPulse(SystemController& controller,
                 std::size_t channel,
                 std::uint32_t timer_tick,
                 std::uint32_t now_ms) noexcept;

void onPwmInput(SystemController& controller,
                std::uint16_t pulse_width_us,
                std::uint32_t now_ms) noexcept;

[[nodiscard]] SystemStepResult step(SystemController& controller,
                                    std::uint32_t now_ms,
                                    float dt_seconds) noexcept;

}  // namespace rpm_sync
