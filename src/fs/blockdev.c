#include "blockdev.h"
#include "../lib/string.h"
#include "../lib/printf.h"

static block_device_t* head = 0;

void blockdev_register(block_device_t* dev) {
    if (!head) {
        head = dev;
        dev->next = 0;
    } else {
        block_device_t* current = head;
        while (current->next) {
            current = current->next;
        }
        current->next = dev;
        dev->next = 0;
    }
    kprintf("blockdev: Registered device '%s' (%d sectors, %d bytes/sec)\n", 
            dev->name, dev->sector_count, dev->sector_size);
}

block_device_t* blockdev_get_first(void) {
    return head;
}

block_device_t* blockdev_find(const char* name) {
    block_device_t* current = head;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return 0;
}
