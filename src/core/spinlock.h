#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>
#include <stdbool.h>

typedef volatile int spinlock_t;

static inline void spinlock_init(spinlock_t* lock) {
    *lock = 0;
}

static inline void spinlock_lock(spinlock_t* lock) {
    while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE)) {
        while (*lock) { 
            __asm__ volatile("pause");
        }
    }
}

static inline void spinlock_unlock(spinlock_t* lock) {
    __atomic_clear(lock, __ATOMIC_RELEASE);
}

#endif
