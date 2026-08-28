#ifndef PWM_OUTPUT_H
#define PWM_OUTPUT_H

#include <stdint.h>

typedef struct {
    uint16_t channel1_us;
    uint16_t channel2_us;
} pwm_output_t;

uint16_t pwm_output_clamp(float requested_us, uint16_t minimum_us, uint16_t maximum_us);
void pwm_output_set(pwm_output_t *output, uint16_t channel1_us, uint16_t channel2_us);

#endif
