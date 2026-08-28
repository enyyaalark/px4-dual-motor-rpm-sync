#include "pwm_input.h"
#include "app_config.h"

void pwm_input_init(pwm_input_t *input) {
    if (input == 0) {
        return;
    }
    input->pulse_width_us = 0U;
    input->last_update_ms = 0U;
    input->valid = false;
}

void pwm_input_update(pwm_input_t *input, uint16_t pulse_width_us, uint32_t now_ms) {
    if (input == 0) {
        return;
    }
    input->pulse_width_us = pulse_width_us;
    input->last_update_ms = now_ms;
    input->valid = (pulse_width_us >= APP_PWM_MIN_US) &&
                   (pulse_width_us <= APP_PWM_MAX_US);
}

bool pwm_input_is_fresh(const pwm_input_t *input, uint32_t now_ms, uint32_t timeout_ms) {
    return (input != 0) && input->valid && (timeout_ms > 0U) &&
           ((uint32_t)(now_ms - input->last_update_ms) <= timeout_ms);
}
