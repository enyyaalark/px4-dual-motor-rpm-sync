#ifndef PWM_INPUT_H
#define PWM_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t pulse_width_us;
    uint32_t last_update_ms;
    bool valid;
} pwm_input_t;

void pwm_input_init(pwm_input_t *input);
void pwm_input_update(pwm_input_t *input, uint16_t pulse_width_us, uint32_t now_ms);
bool pwm_input_is_fresh(const pwm_input_t *input, uint32_t now_ms, uint32_t timeout_ms);

#endif
