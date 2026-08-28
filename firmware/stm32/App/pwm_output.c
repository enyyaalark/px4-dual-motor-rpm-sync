#include "pwm_output.h"

uint16_t pwm_output_clamp(float requested_us, uint16_t minimum_us, uint16_t maximum_us) {
    if (requested_us <= (float)minimum_us) {
        return minimum_us;
    }
    if (requested_us >= (float)maximum_us) {
        return maximum_us;
    }
    return (uint16_t)(requested_us + 0.5F);
}

void pwm_output_set(pwm_output_t *output, uint16_t channel1_us, uint16_t channel2_us) {
    if (output == 0) {
        return;
    }
    output->channel1_us = channel1_us;
    output->channel2_us = channel2_us;
    /* Board-specific HAL timer writes belong in the future hardware adapter. */
}
