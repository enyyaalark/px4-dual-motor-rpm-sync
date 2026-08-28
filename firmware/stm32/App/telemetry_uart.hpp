#pragma once

#include <cstddef>

#include "app_types.hpp"

namespace rpm_sync {

[[nodiscard]] const char* telemetryHeader() noexcept;
[[nodiscard]] int formatTelemetry(char* buffer,
                                  std::size_t buffer_size,
                                  const TelemetrySample& sample) noexcept;

}  // namespace rpm_sync
