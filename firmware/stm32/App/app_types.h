#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

typedef enum {
    APP_STATE_INIT = 0,
    APP_STATE_MONITOR_ONLY,
    APP_STATE_SYNC_CONTROL,
    APP_STATE_BYPASS,
    APP_STATE_FAULT
} app_state_t;

enum {
    APP_FAULT_NONE = 0U,
    APP_FAULT_HALL_1_TIMEOUT = 1U << 0,
    APP_FAULT_HALL_2_TIMEOUT = 1U << 1,
    APP_FAULT_HALL_IMPLAUSIBLE = 1U << 2,
    APP_FAULT_PWM_INPUT_INVALID = 1U << 3,
    APP_FAULT_OUTPUT_SATURATED = 1U << 4
};

typedef struct {
    uint32_t timestamp_ms;
    uint16_t base_pwm_us;
    float rpm1;
    float rpm2;
    float error_rpm;
    float error_percent;
    float correction_us;
    uint16_t pwm1_us;
    uint16_t pwm2_us;
    app_state_t state;
    uint32_t fault_flags;
} app_telemetry_sample_t;

#endif
