#include "sync_controller.hpp"

#include <cmath>

namespace rpm_sync {
namespace {

float absolute(float value) noexcept {
    return value < 0.0F ? -value : value;
}

float clampSymmetric(float value, float limit) noexcept {
    limit = absolute(limit);
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

float positiveLimit(float value) noexcept {
    return absolute(value);
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
    if (!enable || !std::isfinite(rpm1) || !std::isfinite(rpm2) ||
        !std::isfinite(dt_seconds) ||
        (dt_seconds <= 0.0F) ||
        (rpm1 < config.minimum_rpm) || (rpm2 < config.minimum_rpm) ||
        !std::isfinite(config.kp) || !std::isfinite(config.ki) ||
        !std::isfinite(config.deadband_rpm) ||
        !std::isfinite(config.minimum_rpm) ||
        !std::isfinite(config.correction_limit_us) ||
        !std::isfinite(config.integral_limit) ||
        (positiveLimit(config.correction_limit_us) <= 0.0F)) {
        reset(controller);
        return 0.0F;
    }

    float error = rpm1 - rpm2;
    if (absolute(error) <= positiveLimit(config.deadband_rpm)) {
        error = 0.0F;
    }

    const float previous_integral = controller.integral;
    const float integral_limit = positiveLimit(config.integral_limit);
    const float candidate_integral = clampSymmetric(
        previous_integral + error * dt_seconds, integral_limit);
    const float correction_limit = positiveLimit(config.correction_limit_us);
    const float candidate_correction =
        config.kp * error + config.ki * candidate_integral;
    const float limited_correction =
        clampSymmetric(candidate_correction, correction_limit);

    // Conditional integration: retain the previous integral when the new
    // error would drive an already saturated output farther into saturation.
    const bool drives_positive_saturation =
        (candidate_correction > correction_limit) && (error > 0.0F);
    const bool drives_negative_saturation =
        (candidate_correction < -correction_limit) && (error < 0.0F);
    if (drives_positive_saturation || drives_negative_saturation) {
        controller.integral = previous_integral;
        return clampSymmetric(config.kp * error + config.ki * previous_integral,
                              correction_limit);
    }

    controller.integral = candidate_integral;
    return limited_correction;
}

}  // namespace rpm_sync
