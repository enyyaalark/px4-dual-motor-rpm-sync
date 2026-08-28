#pragma once

#include <cstdint>

namespace rpm_sync {

enum class AppState : std::uint8_t {
    kInit = 0,
    kMonitorOnly,
    kSyncControl,
    kBypass,
    kFault,
};

enum class FaultFlag : std::uint32_t {
    kNone = 0U,
    kHall1Timeout = 1U << 0,
    kHall2Timeout = 1U << 1,
    kHallImplausible = 1U << 2,
    kPwmInputInvalid = 1U << 3,
    kOutputSaturated = 1U << 4,
};

[[nodiscard]] constexpr std::uint32_t toMask(FaultFlag flag) noexcept {
    return static_cast<std::uint32_t>(flag);
}

struct TelemetrySample {
    std::uint32_t timestamp_ms{};
    std::uint16_t base_pwm_us{};
    float rpm1{};
    float rpm2{};
    float error_rpm{};
    float error_percent{};
    float correction_us{};
    std::uint16_t pwm1_us{};
    std::uint16_t pwm2_us{};
    AppState state{AppState::kInit};
    std::uint32_t fault_flags{};
};

}  // namespace rpm_sync
