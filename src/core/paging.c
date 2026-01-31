#include "paging.h"
#include "../lib/memory.h"
#include "../lib/printf.h"

/* 
   Placeholder for 64-bit Paging.
   Currently relies on the Identity Map set up by boot_64.s
   and shares it across all processes.
*/

static page_directory_t* kernel_pml4 = 0;

void paging_init(void) {
    kprintf("paging64: using bootloader identity map.\n");
    // We don't overwrite the page tables here yet.
}

void switch_page_directory(page_directory_t* dir) {
    // Only switch if different (and valid)
    if (dir) {
        __asm__ __volatile__("mov %0, %%cr3" :: "r"(dir));
    }
}

page_directory_t* paging_create_directory(void) {
    // Return current CR3 (Shared Paging for now)
    return (page_directory_t*)__asm_get_cr3();
}

void paging_map(page_directory_t* dir, uint64_t vaddr, uint64_t paddr, int user) {
    UNUSED(dir); UNUSED(vaddr); UNUSED(paddr); UNUSED(user);
    // TODO: Implement 4-level paging map
}

uint64_t __asm_get_cr3(void) {
    uint64_t cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void paging_map_mmio(uint64_t paddr, uint64_t size) {
    UNUSED(paddr); UNUSED(size);
    // Identity mapped by bootloader for now
}
