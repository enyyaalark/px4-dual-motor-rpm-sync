#ifndef BYPASS_CONTROL_H
#define BYPASS_CONTROL_H

#include <stdbool.h>

typedef struct {
    bool self_test_complete;
    bool bypass_requested;
} bypass_control_t;

void bypass_control_init(bypass_control_t *control);
bool bypass_control_use_corrected_pwm(const bypass_control_t *control, bool has_fault);

#endif
