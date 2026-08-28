#include "bypass_control.hpp"

namespace rpm_sync {

void reset(BypassControl& control) noexcept {
    control = {};
    control.bypass_requested = true;
}

bool useCorrectedPwm(const BypassControl& control, bool has_fault) noexcept {
    return control.self_test_complete && !control.bypass_requested && !has_fault;
}

}  // namespace rpm_sync
