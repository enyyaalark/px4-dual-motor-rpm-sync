#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t active_flags;
    uint32_t latched_flags;
} fault_manager_t;

void fault_manager_init(fault_manager_t *manager);
void fault_manager_set(fault_manager_t *manager, uint32_t flag, bool latch);
void fault_manager_clear_active(fault_manager_t *manager, uint32_t flag);
bool fault_manager_has_fault(const fault_manager_t *manager);

#endif
