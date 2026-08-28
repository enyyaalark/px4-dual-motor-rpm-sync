#pragma once

#include <cstdint>

namespace rpm_sync::config {

// Provisional protocol bounds; replace only after ESC calibration.
inline constexpr std::uint16_t kPwmMinUs = 1000U;
inline constexpr std::uint16_t kPwmMaxUs = 2000U;

// Closed-loop control remains disabled until hardware calibration.
inline constexpr bool kSyncControlDefaultOn = false;
inline constexpr float kPulsesPerRevolution = 0.0F;
inline constexpr float kKpDefault = 0.0F;
inline constexpr float kKiDefault = 0.0F;
inline constexpr float kDeadbandRpmDefault = 0.0F;
inline constexpr float kMinClosedLoopRpm = 0.0F;
inline constexpr float kCorrectionLimitUs = 0.0F;
inline constexpr float kIntegralLimit = 0.0F;

// Timing values require measured pulse and control rates.
inline constexpr std::uint32_t kHallTimeoutMs = 0U;
inline constexpr std::uint32_t kPwmInputTimeoutMs = 0U;
inline constexpr std::uint32_t kTelemetryPeriodMs = 0U;

}  // namespace rpm_sync::config
