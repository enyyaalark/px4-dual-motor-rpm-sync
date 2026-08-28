#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Provisional protocol bounds; replace only after ESC calibration. */
#define APP_PWM_MIN_US                 1000U
#define APP_PWM_MAX_US                 2000U

/* Closed-loop control remains disabled until hardware calibration. */
#define APP_SYNC_CONTROL_DEFAULT_ON    0
#define APP_PULSES_PER_REVOLUTION      0.0F
#define APP_KP_DEFAULT                 0.0F
#define APP_KI_DEFAULT                 0.0F
#define APP_DEADBAND_RPM_DEFAULT       0.0F
#define APP_MIN_CLOSED_LOOP_RPM        0.0F
#define APP_CORRECTION_LIMIT_US        0.0F
#define APP_INTEGRAL_LIMIT             0.0F

/* Timing values require measured pulse and control rates. */
#define APP_HALL_TIMEOUT_MS            0U
#define APP_PWM_INPUT_TIMEOUT_MS       0U
#define APP_TELEMETRY_PERIOD_MS        0U

#endif
