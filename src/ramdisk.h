#ifndef RAMDISK_H
#define RAMDISK_H

#include "common.h"

/* Simple linear RAM-backed block device abstraction.
   For now this is just a header + API; integration with a real
   ext2 image will come next. */

void ramdisk_init(uint8_t* base, uint32_t size);
int  ramdisk_read(uint32_t offset, void* buf, uint32_t len);
uint32_t ramdisk_size(void);

#endif
