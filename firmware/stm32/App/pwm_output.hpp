#pragma once

#include <cstdint>

namespace rpm_sync {

struct PwmOutput {
    std::uint16_t channel1_us{};
    std::uint16_t channel2_us{};
};

[[nodiscard]] std::uint16_t clampPwm(float requested_us,
                                     std::uint16_t minimum_us,
                                     std::uint16_t maximum_us) noexcept;
void set(PwmOutput& output,
         std::uint16_t channel1_us,
         std::uint16_t channel2_us) noexcept;

}  // namespace rpm_sync
