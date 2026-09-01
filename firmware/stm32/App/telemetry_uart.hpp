#pragma once

#include <cstddef>
#include <cstdint>

#include "app_types.hpp"

namespace rpm_sync {

enum class TelemetryFormatStatus : std::uint8_t {
    kOk,
    kInvalidArgument,
    kInvalidSample,
    kBufferTooSmall,
    kFormatError,
};

struct TelemetryFormatResult {
    std::size_t length{};
    TelemetryFormatStatus status{TelemetryFormatStatus::kInvalidArgument};
};

[[nodiscard]] const char* telemetryHeader() noexcept;
[[nodiscard]] TelemetryFormatResult formatTelemetry(
    char* buffer,
    std::size_t buffer_size,
    const TelemetrySample& sample) noexcept;

}  // namespace rpm_sync
