#ifndef PROCESS_H
#define PROCESS_H

#include "common.h"
#include "isr.h"

#define MAX_PROCESSES 32

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_STOPPED,
    PROC_WAITING
} proc_state_t;

typedef struct {
    uint32_t pid;
    uint64_t entry_point;
    uint64_t stack_pointer;   // Initial stack top
    uint64_t page_directory;
    proc_state_t state;
    char name[32];
    uint64_t kernel_stack;    // Kernel stack for this process
    uint64_t rsp;             // Saved stack pointer (points to registers)
    int is_userland;
} process_t;

extern process_t* current_process;

/* Initialize process manager */
void process_init(void);

/* Create a new process */
process_t* process_create(const char* name, uint64_t entry_point);

/* Execute a process (simple jump for now) */
void process_execute(process_t* proc);
void process_load_and_execute(process_t* proc, const void* data, size_t size);

#endif
