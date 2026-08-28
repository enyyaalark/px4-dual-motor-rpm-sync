#include "fault_manager.hpp"

namespace rpm_sync {

void reset(FaultManager& manager) noexcept {
    manager = {};
}

void setFault(FaultManager& manager, FaultFlag flag, bool latch) noexcept {
    const auto mask = toMask(flag);
    manager.active_flags |= mask;
    if (latch) {
        manager.latched_flags |= mask;
    }
}

void clearActiveFault(FaultManager& manager, FaultFlag flag) noexcept {
    manager.active_flags &= ~toMask(flag);
}

bool hasFault(const FaultManager& manager) noexcept {
    return (manager.active_flags | manager.latched_flags) != 0U;
}

}  // namespace rpm_sync
