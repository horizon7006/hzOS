#ifndef PAGING_H
#define PAGING_H

#include "common.h"

typedef struct {
    uint64_t entries[512];
} page_directory_t;

typedef struct {
    uint64_t entries[512];
} page_table_t;

void paging_init(void);
void switch_page_directory(page_directory_t* dir);

/* Clone a page directory (useful for creating process spaces) */
page_directory_t* paging_create_directory(void);

/* Map a virtual page to a physical address */
void paging_map(page_directory_t* dir, uint64_t vaddr, uint64_t paddr, int user);

/* Map MMIO range (identity mapped for now) */
void paging_map_mmio(uint64_t paddr, uint64_t size);

uint64_t __asm_get_cr3(void);

#endif
