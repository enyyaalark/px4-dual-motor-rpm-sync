#include "telemetry_uart.h"
#include <stdio.h>

static const char *state_name(app_state_t state) {
    switch (state) {
        case APP_STATE_INIT: return "INIT";
        case APP_STATE_MONITOR_ONLY: return "MONITOR_ONLY";
        case APP_STATE_SYNC_CONTROL: return "SYNC_CONTROL";
        case APP_STATE_BYPASS: return "BYPASS";
        case APP_STATE_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

const char *telemetry_uart_header(void) {
    return "timestamp_ms,base_pwm_us,rpm1,rpm2,error_rpm,error_percent,"
           "correction_us,pwm1_us,pwm2_us,system_state,fault_flags\n";
}

int telemetry_uart_format(char *buffer,
                          size_t buffer_size,
                          const app_telemetry_sample_t *sample) {
    if ((buffer == 0) || (buffer_size == 0U) || (sample == 0)) {
        return -1;
    }
    return snprintf(buffer, buffer_size,
                    "%lu,%u,%.2f,%.2f,%.2f,%.3f,%.2f,%u,%u,%s,0x%04lX\n",
                    (unsigned long)sample->timestamp_ms,
                    (unsigned int)sample->base_pwm_us,
                    (double)sample->rpm1,
                    (double)sample->rpm2,
                    (double)sample->error_rpm,
                    (double)sample->error_percent,
                    (double)sample->correction_us,
                    (unsigned int)sample->pwm1_us,
                    (unsigned int)sample->pwm2_us,
                    state_name(sample->state),
                    (unsigned long)sample->fault_flags);
}
