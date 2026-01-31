#include "elf.h"
#include "../lib/memory.h"
#include "../lib/printf.h"
#include "paging.h"

/* Validate ELF header */
int elf_validate(const void* elf_data) {
    if (!elf_data) return -1;
    
    const elf_header_t* hdr = (const elf_header_t*)elf_data;
    
    // Check magic number
    if (hdr->e_ident[0] != 0x7F || hdr->e_ident[1] != 'E' ||
        hdr->e_ident[2] != 'L' || hdr->e_ident[3] != 'F') {
        kprintf("elf: invalid magic\n");
        return -1;
    }
    
    // Check 32-bit
    if (hdr->e_ident[4] != ELF_CLASS_32) {
        kprintf("elf: not 32-bit\n");
        return -1;
    }
    
    // Check executable
    if (hdr->e_type != ELF_TYPE_EXEC) {
        kprintf("elf: not executable\n");
        return -1;
    }
    
    // Check x86
    if (hdr->e_machine != ELF_MACHINE_386) {
        kprintf("elf: not x86\n");
        return -1;
    }
    
    return 0;
}

/* Get entry point */
uint32_t elf_get_entry(const void* elf_data) {
    const elf_header_t* hdr = (const elf_header_t*)elf_data;
    return hdr->e_entry;
}

/* Load ELF into memory and return entry point */
int elf_load(const void* elf_data, size_t size) {
    if (elf_validate(elf_data) != 0) {
        return -1;
    }
    
    const elf_header_t* hdr = (const elf_header_t*)elf_data;
    const uint8_t* data = (const uint8_t*)elf_data;
    
    kprintf("elf: entry point = 0x%x\n", hdr->e_entry);
    kprintf("elf: program headers = %d\n", hdr->e_phnum);
    
    // Process program headers
    for (int i = 0; i < hdr->e_phnum; i++) {
        const elf_program_header_t* phdr = 
            (const elf_program_header_t*)(data + hdr->e_phoff + i * hdr->e_phentsize);
        
        if (phdr->p_type == PT_LOAD) {
            // Allocate physical memory for this segment and map it
            // We'll simplify: map vaddr to vaddr in the *current* process directory
            // Note: Since we switch directory in process_execute, we should probably 
            // map them here. 
            uint32_t num_pages = (phdr->p_memsz + 4095) / 4096;
            for (uint32_t j = 0; j < num_pages; j++) {
                uint32_t page_vaddr = phdr->p_vaddr + j * 4096;
                // Allocate a fresh physical page for this process
                // Use raw aligned to avoid heap corruption from headers
                void* phys_page = kmalloc_raw_aligned(4096); 
                paging_map((page_directory_t*)__asm_get_cr3(), page_vaddr, (uint32_t)phys_page, 1);
            }

            void* dest = (void*)phdr->p_vaddr;
            memset(dest, 0, phdr->p_memsz);
            if (phdr->p_filesz > 0) {
                memcpy(dest, data + phdr->p_offset, phdr->p_filesz);
            }
        }
    }
    
    return 0;
}
