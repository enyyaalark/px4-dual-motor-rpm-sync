#pragma once

#include <cstdint>

namespace rpm_sync {

[[nodiscard]] bool hallTimedOut(std::uint32_t now_ms,
                                std::uint32_t last_pulse_ms,
                                std::uint32_t timeout_ms) noexcept;
[[nodiscard]] bool rpmPlausible(float rpm, float maximum_rpm) noexcept;

}  // namespace rpm_sync
