#pragma once

namespace rpm_sync {

struct SyncControllerConfig {
    float kp{};
    float ki{};
    float deadband_rpm{};
    float minimum_rpm{};
    float correction_limit_us{};
    float integral_limit{};
};

struct SyncController {
    float integral{};
};

void reset(SyncController& controller) noexcept;
[[nodiscard]] float step(SyncController& controller,
                         const SyncControllerConfig& config,
                         float rpm1,
                         float rpm2,
                         float dt_seconds,
                         bool enable) noexcept;

}  // namespace rpm_sync
