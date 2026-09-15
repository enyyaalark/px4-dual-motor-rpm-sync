#pragma once

#include <cstdint>

namespace rpm_sync::config {

// PX4 1.17 MAIN1/MAIN2 were measured at 1000 us disarmed and 1300 us under
// actuator_test, with configured active endpoints of 1100..1900 us. These
// MONITOR_ONLY plausibility bounds retain 50 us margin around 1000..1900 us.
inline constexpr std::uint16_t kPwmInputMinUs = 950U;
inline constexpr std::uint16_t kPwmInputMaxUs = 1'950U;

// Bench-only ESC output bounds (v2, 2026-09-15). Field-measured working point
// is about 1065 us (~2840 RPM) with a safe band 1060..1080 us; this keeps the
// corrected path inside the confirmed no-prop bench range.
inline constexpr std::uint16_t kPwmOutputMinUs = 1060U;
inline constexpr std::uint16_t kPwmOutputMaxUs = 1080U;
// Maximum allowed difference between the two base PWM inputs. Both MAIN1 and
// MAIN2 carry the same base command in phase 1; a larger mismatch keeps the
// corrected path disabled.
inline constexpr std::uint16_t kBasePwmMismatchUs = 20U;

// PPR was established from the 2026-09-10 two-motor, three-point calibration.
// The Hall timeout and maximum RPM are engineering initial values accepted by the
// team for MONITOR_ONLY validation. They do not raise the 3000 RPM bench limit or
// enable closed-loop control and must be re-reviewed after the dual-channel test.
inline constexpr bool kSyncControlDefaultOn = false;
inline constexpr float kHallTimerHz = 1'000'000.0F;
inline constexpr float kPulsesPerRevolution = 1.0F;
inline constexpr float kMaximumRpm = 3'300.0F;
// Initial bench P-only gains. These are starting points for the demo and must
// be re-reviewed after dual-Hall RPM evidence is collected.
inline constexpr float kKpDefault = 0.05F;
inline constexpr float kKiDefault = 0.0F;
inline constexpr float kDeadbandRpmDefault = 10.0F;
inline constexpr float kMinClosedLoopRpm = 1000.0F;
inline constexpr float kCorrectionLimitUs = 40.0F;
inline constexpr float kIntegralLimit = 0.0F;

// 100 ms is an engineering initial value based on the longest selected measured
// period (34.787 ms). VALID -> TIMED_OUT and RPM zeroing remain to be tested.
inline constexpr std::uint32_t kHallTimeoutMs = 100U;
// Four missing 400 Hz frames exceed 10 ms. This is an initial MONITOR_ONLY
// timeout and does not enable closed-loop control.
inline constexpr std::uint32_t kPwmInputTimeoutMs = 10U;
inline constexpr std::uint32_t kTelemetryPeriodMs = 0U;

}  // namespace rpm_sync::config
