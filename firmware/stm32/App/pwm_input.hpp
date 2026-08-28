#pragma once

#include <cstdint>

namespace rpm_sync {

struct PwmInput {
    std::uint16_t pulse_width_us{};
    std::uint32_t last_update_ms{};
    bool valid{};
};

void reset(PwmInput& input) noexcept;
void update(PwmInput& input, std::uint16_t pulse_width_us, std::uint32_t now_ms) noexcept;
[[nodiscard]] bool isFresh(const PwmInput& input,
                           std::uint32_t now_ms,
                           std::uint32_t timeout_ms) noexcept;

}  // namespace rpm_sync
