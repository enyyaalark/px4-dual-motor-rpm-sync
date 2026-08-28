#include "sync_controller.hpp"

namespace rpm_sync {
namespace {

float clampSymmetric(float value, float limit) noexcept {
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

float absolute(float value) noexcept {
    return value < 0.0F ? -value : value;
}

}  // namespace

void reset(SyncController& controller) noexcept {
    controller.integral = 0.0F;
}

float step(SyncController& controller,
           const SyncControllerConfig& config,
           float rpm1,
           float rpm2,
           float dt_seconds,
           bool enable) noexcept {
    if (!enable || (dt_seconds <= 0.0F) ||
        (rpm1 < config.minimum_rpm) || (rpm2 < config.minimum_rpm) ||
        (config.correction_limit_us <= 0.0F)) {
        reset(controller);
        return 0.0F;
    }

    float error = rpm1 - rpm2;
    if (absolute(error) <= config.deadband_rpm) {
        error = 0.0F;
    }

    controller.integral = clampSymmetric(
        controller.integral + error * dt_seconds, config.integral_limit);
    return clampSymmetric(config.kp * error + config.ki * controller.integral,
                          config.correction_limit_us);
}

}  // namespace rpm_sync
