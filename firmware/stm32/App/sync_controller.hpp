#pragma once

namespace rpm_sync {

struct SyncControllerConfig {
    float kp{};
    float ki{};
    float deadband_rpm{};
    float minimum_rpm{};
    float correction_limit_us{};
    float integral_limit{};
    float kd{};
    float filter_tau_seconds{};
};

struct SyncController {
    float integral{};
    float filtered_error{};
    bool has_previous_error{};
};

void reset(SyncController& controller) noexcept;
[[nodiscard]] float step(SyncController& controller,
                         const SyncControllerConfig& config,
                         float rpm1,
                         float rpm2,
                         float dt_seconds,
                         bool enable) noexcept;

}  // namespace rpm_sync
