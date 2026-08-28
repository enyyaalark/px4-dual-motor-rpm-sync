#include "sync_controller.h"

static float clamp_symmetric(float value, float limit) {
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

static float absolute(float value) {
    return value < 0.0F ? -value : value;
}

void sync_controller_reset(sync_controller_t *controller) {
    if (controller != 0) {
        controller->integral = 0.0F;
    }
}

float sync_controller_step(sync_controller_t *controller,
                           const sync_controller_config_t *config,
                           float rpm1,
                           float rpm2,
                           float dt_seconds,
                           bool enable) {
    if ((controller == 0) || (config == 0) || !enable || (dt_seconds <= 0.0F) ||
        (rpm1 < config->minimum_rpm) || (rpm2 < config->minimum_rpm) ||
        (config->correction_limit_us <= 0.0F)) {
        sync_controller_reset(controller);
        return 0.0F;
    }

    float error = rpm1 - rpm2;
    if (absolute(error) <= config->deadband_rpm) {
        error = 0.0F;
    }

    controller->integral = clamp_symmetric(controller->integral + error * dt_seconds,
                                            config->integral_limit);
    return clamp_symmetric(config->kp * error + config->ki * controller->integral,
                           config->correction_limit_us);
}
