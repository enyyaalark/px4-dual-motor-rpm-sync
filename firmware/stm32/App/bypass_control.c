#include "bypass_control.h"

void bypass_control_init(bypass_control_t *control) {
    if (control != 0) {
        control->self_test_complete = false;
        control->bypass_requested = true;
    }
}

bool bypass_control_use_corrected_pwm(const bypass_control_t *control, bool has_fault) {
    return (control != 0) && control->self_test_complete &&
           !control->bypass_requested && !has_fault;
}
