#pragma once

#include <cstdint>

namespace rpm_sync {

struct RpmCapture {
    std::uint32_t previous_tick{};
    std::uint32_t period_ticks{};
    std::uint32_t last_pulse_ms{};
    bool has_period{};
};

void reset(RpmCapture& capture) noexcept;
void onPulse(RpmCapture& capture, std::uint32_t timer_tick, std::uint32_t now_ms) noexcept;
[[nodiscard]] bool calculateRpm(const RpmCapture& capture,
                                float timer_hz,
                                float pulses_per_revolution,
                                float& rpm_out) noexcept;

}  // namespace rpm_sync
