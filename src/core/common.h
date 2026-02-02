#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>

#define UNUSED(x) (void)(x)

extern uint64_t g_hhdm_offset;
#define PHYS_TO_VIRT(addr) ((uintptr_t)(addr) + g_hhdm_offset)
#define VIRT_TO_PHYS(addr) ((uintptr_t)(addr) - g_hhdm_offset)

#endif