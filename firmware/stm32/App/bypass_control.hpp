#pragma once

namespace rpm_sync {

struct BypassControl {
    bool self_test_complete{};
    bool bypass_requested{true};
};

void reset(BypassControl& control) noexcept;
[[nodiscard]] bool useCorrectedPwm(const BypassControl& control,
                                   bool has_fault) noexcept;

}  // namespace rpm_sync
