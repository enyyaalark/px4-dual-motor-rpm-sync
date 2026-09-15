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

bool haveOppositeSigns(float lhs, float rhs) noexcept {
    return ((lhs > 0.0F) && (rhs < 0.0F)) ||
           ((lhs < 0.0F) && (rhs > 0.0F));
}

float filterAlpha(float dt_seconds, float tau_seconds) noexcept {
    // Form the ratio with the smaller value in the numerator so that two
    // large, finite inputs cannot overflow when added together.
    if (dt_seconds >= tau_seconds) {
        return 1.0F / (1.0F + tau_seconds / dt_seconds);
    }
    const float ratio = dt_seconds / tau_seconds;
    return ratio / (1.0F + ratio);
}

}  // namespace

void reset(SyncController& controller) noexcept {
    controller.integral = 0.0F;
    controller.filtered_error = 0.0F;
    controller.has_previous_error = false;
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
        !std::isfinite(config.kd) ||
        !std::isfinite(config.filter_tau_seconds) ||
        !std::isfinite(controller.integral) ||
        !std::isfinite(controller.filtered_error) ||
        (positiveLimit(config.correction_limit_us) <= 0.0F)) {
        reset(controller);
        return 0.0F;
    }

    float error = rpm1 - rpm2;
    if (!std::isfinite(error)) {
        reset(controller);
        return 0.0F;
    }

    const float deadband = positiveLimit(config.deadband_rpm);
    if (error > deadband) {
        error -= deadband;
    } else if (error < -deadband) {
        error += deadband;
    } else {
        error = 0.0F;
    }

    // Once both motors are inside the accepted band, remove stored bias so a
    // filtered or integral tail cannot push the error through the setpoint.
    if (error == 0.0F) {
        reset(controller);
        return 0.0F;
    }

    // A sampled reversal can jump across the deadband. Seed the filter with
    // the current error so the proportional path responds in the new
    // direction immediately, while suppressing a derivative kick for this
    // first reversed sample. The conditional integrator below remains
    // responsible for stored bias.
    if (controller.has_previous_error &&
        haveOppositeSigns(error, controller.filtered_error)) {
        controller.filtered_error = error;
        controller.has_previous_error = false;
    }

    float filtered_error = error;
    if (config.filter_tau_seconds > 0.0F) {
        const float alpha =
            filterAlpha(dt_seconds, config.filter_tau_seconds);
        filtered_error = alpha * error +
                         (1.0F - alpha) * controller.filtered_error;
    }

    const float derivative =
        controller.has_previous_error
            ? (filtered_error - controller.filtered_error) / dt_seconds
            : 0.0F;
    controller.filtered_error = filtered_error;
    controller.has_previous_error = true;

    const float previous_integral = controller.integral;
    const float integral_limit = positiveLimit(config.integral_limit);
    const float candidate_integral = clampSymmetric(
        previous_integral + error * dt_seconds, integral_limit);
    const float correction_limit = positiveLimit(config.correction_limit_us);
    const float proportional = config.kp * filtered_error;
    const float derivative_term = config.kd * derivative;
    const float integral_term = config.ki * candidate_integral;
    if (!std::isfinite(filtered_error) || !std::isfinite(derivative) ||
        !std::isfinite(proportional) || !std::isfinite(derivative_term) ||
        !std::isfinite(integral_term)) {
        reset(controller);
        return 0.0F;
    }

    const float candidate_correction =
        proportional + integral_term + derivative_term;
    if (!std::isfinite(candidate_correction)) {
        reset(controller);
        return 0.0F;
    }

    const float limited_correction =
        clampSymmetric(candidate_correction, correction_limit);

    // Conditional integration: retain the previous integral when the new
    // error would drive an already saturated output farther into saturation.
    const bool drives_positive_saturation =
        (candidate_correction > correction_limit) && (error > 0.0F);
    const bool drives_negative_saturation =
        (candidate_correction < -correction_limit) && (error < 0.0F);
    if (drives_positive_saturation || drives_negative_saturation) {
        const float previous_integral_term =
            config.ki * previous_integral;
        const float held_correction =
            proportional + previous_integral_term + derivative_term;
        if (!std::isfinite(previous_integral_term) ||
            !std::isfinite(held_correction)) {
            reset(controller);
            return 0.0F;
        }

        controller.integral = previous_integral;
        return clampSymmetric(held_correction, correction_limit);
    }

    controller.integral = candidate_integral;
    return limited_correction;
}

}  // namespace rpm_sync
