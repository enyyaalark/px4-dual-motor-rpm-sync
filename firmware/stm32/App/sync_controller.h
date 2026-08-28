#ifndef SYNC_CONTROLLER_H
#define SYNC_CONTROLLER_H

#include <stdbool.h>

typedef struct {
    float kp;
    float ki;
    float deadband_rpm;
    float minimum_rpm;
    float correction_limit_us;
    float integral_limit;
} sync_controller_config_t;

typedef struct {
    float integral;
} sync_controller_t;

void sync_controller_reset(sync_controller_t *controller);
float sync_controller_step(sync_controller_t *controller,
                           const sync_controller_config_t *config,
                           float rpm1,
                           float rpm2,
                           float dt_seconds,
                           bool enable);

#endif
