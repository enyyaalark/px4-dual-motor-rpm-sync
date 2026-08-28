#include "fault_manager.h"

void fault_manager_init(fault_manager_t *manager) {
    if (manager != 0) {
        manager->active_flags = 0U;
        manager->latched_flags = 0U;
    }
}

void fault_manager_set(fault_manager_t *manager, uint32_t flag, bool latch) {
    if (manager == 0) {
        return;
    }
    manager->active_flags |= flag;
    if (latch) {
        manager->latched_flags |= flag;
    }
}

void fault_manager_clear_active(fault_manager_t *manager, uint32_t flag) {
    if (manager != 0) {
        manager->active_flags &= ~flag;
    }
}

bool fault_manager_has_fault(const fault_manager_t *manager) {
    return (manager != 0) && ((manager->active_flags | manager->latched_flags) != 0U);
}
