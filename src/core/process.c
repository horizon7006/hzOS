#include "process.h"
#include "../lib/memory.h"
#include "../lib/printf.h"
#include "syscall.h"
#include "paging.h"
#include "elf.h"

static process_t processes[MAX_PROCESSES];
static uint32_t next_pid = 1;
process_t* current_process = NULL;

void process_init(void) {
    memset(processes, 0, sizeof(processes));
    
    // Create 'kernel' process for the main thread
    process_t* kproc = &processes[0];
    kproc->pid = next_pid++;
    kproc->state = PROC_RUNNING;
    
    // Manual copy of "kernel" name
    kproc->name[0] = 'k'; kproc->name[1] = 'e'; kproc->name[2] = 'r';
    kproc->name[3] = 'n'; kproc->name[4] = 'e'; kproc->name[5] = 'l';
    kproc->name[6] = '\0';
    
    kproc->page_directory = __asm_get_cr3();
    kproc->is_userland = 0;
    current_process = kproc;

    kprintf("process: multi-tasking enabled (kernel process pid=1)\n");
}

process_t* process_create(const char* name, uint64_t entry_point) {
    // Find free slot
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_UNUSED) {
            process_t* proc = &processes[i];
            proc->pid = next_pid++;
            proc->entry_point = entry_point;
            proc->state = PROC_READY;
            
            // Copy name
            int j = 0;
            while (name[j] && j < 31) {
                proc->name[j] = name[j];
                j++;
            }
            proc->name[j] = '\0';
            
            // Allocate stack (8KB for multitasking safety)
            proc->stack_pointer = (uint64_t)kmalloc_a(8192) + 8192;
            
            // Setup paging subdirectory (Use shared for now)
            proc->page_directory = (uint64_t)paging_create_directory();
            
            // kprintf("process: created pid=%d name=%s entry=0x%lx\n", 
            //        proc->pid, proc->name, entry_point);
            return proc;
        }
    }
    
    kprintf("process: no free process slots\n");
    return NULL;
}

struct registers* scheduler_schedule(struct registers* regs) {
    if (!current_process) return regs;

    // Save current process stack pointer
    current_process->rsp = (uint64_t)regs;

    // Pick next process (Simple Round Robin)
    int current_idx = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (&processes[i] == current_process) {
            current_idx = i;
            break;
        }
    }

    process_t* next = NULL;
    for (int i = 1; i <= MAX_PROCESSES; i++) {
        int idx = (current_idx + i) % MAX_PROCESSES;
        if (processes[idx].state == PROC_RUNNING || processes[idx].state == PROC_READY) {
            next = &processes[idx];
            break;
        }
    }

    if (next && next != current_process) {
        current_process = next;
        next->state = PROC_RUNNING;
        
        // Switch address space
        if (__asm_get_cr3() != next->page_directory) {
            switch_page_directory((page_directory_t*)next->page_directory);
        }
        
        return (struct registers*)next->rsp;
    }

    return regs;
}

void process_execute(process_t* proc) {
    if (!proc) return;
    
    // In a multi-tasking system, 'execute' just marks as ready and waits for scheduler
    proc->state = PROC_READY;
    
    // Setup initial stack for switching
    // We need to 'push' an initial register state onto its stack
    // Since we identity map the stack, we can write to it here
    // Setup initial stack for switching
    // We need to 'push' an initial register state onto its stack
    struct registers* regs = (struct registers*)(proc->stack_pointer - sizeof(struct registers));
    memset(regs, 0, sizeof(struct registers));
    
    regs->rip = proc->entry_point;
    regs->cs = 0x08;
    regs->ds = 0x10;
    regs->ss = 0x10;
    regs->rflags = 0x202; // IF = 1
    regs->rsp = proc->stack_pointer;
    
    proc->rsp = (uint64_t)regs;
    
    // kprintf("process: pid=%d queued for execution\n", proc->pid);
}

void process_load_and_execute(process_t* proc, const void* data, size_t size) {
    if (!proc || !data) return;
    
    // Temporarily switch to process address space to load ELF
    uint64_t old_cr3 = __asm_get_cr3();
    switch_page_directory((page_directory_t*)proc->page_directory);
    
    if (elf_load(data, size) != 0) {
        kprintf("process: failed to load ELF segments\n");
        switch_page_directory((page_directory_t*)old_cr3);
        return;
    }
    
    switch_page_directory((page_directory_t*)old_cr3);
    process_execute(proc);
}
