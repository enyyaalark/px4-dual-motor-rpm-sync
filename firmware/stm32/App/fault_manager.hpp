#pragma once

#include <cstdint>

#include "app_types.hpp"

namespace rpm_sync {

struct FaultManager {
    std::uint32_t active_flags{};
    std::uint32_t latched_flags{};
};

void reset(FaultManager& manager) noexcept;
void setFault(FaultManager& manager, FaultFlag flag, bool latch) noexcept;
void clearActiveFault(FaultManager& manager, FaultFlag flag) noexcept;
[[nodiscard]] bool hasFault(const FaultManager& manager) noexcept;

}  // namespace rpm_sync
