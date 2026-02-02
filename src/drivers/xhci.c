#include "xhci.h"
#include "../lib/memory.h"
#include "../lib/printf.h"
#include "../core/io.h"

static xhci_controller_t g_xhci;

static inline void xhci_write_op32(xhci_controller_t* xhci, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(xhci->op_base + reg) = val;
}

static inline uint32_t xhci_read_op32(xhci_controller_t* xhci, uint32_t reg) {
    return *(volatile uint32_t*)(xhci->op_base + reg);
}

static void xhci_ring_doorbell(xhci_controller_t* xhci, uint32_t target, uint32_t stream_id) {
    *(volatile uint32_t*)(xhci->db_base + target * 4) = stream_id;
}

static int xhci_submit_cmd(xhci_controller_t* xhci, xhci_trb_t* cmd) {
    uint32_t index = xhci->cmd_ring_index;
    xhci_trb_t* slot = &xhci->cmd_ring[index];
    
    // Copy but keep cycle bit for last
    slot->parameter = cmd->parameter;
    slot->status = cmd->status;
    uint32_t control = cmd->control & ~1; // Clear cycle bit
    control |= xhci->cmd_cycle;
    
    slot->control = control;

    xhci->cmd_ring_index++;
    if (xhci->cmd_ring_index >= 63) { // 64th is Link TRB
        xhci_trb_t* link = &xhci->cmd_ring[63];
        link->parameter = VIRT_TO_PHYS(xhci->cmd_ring);
        link->status = 0;
        link->control = (6 << 10) | (1 << 1) | xhci->cmd_cycle; // Link type, TC=1
        
        xhci->cmd_ring_index = 0;
        xhci->cmd_cycle ^= 1;
    }

    xhci_ring_doorbell(xhci, 0, 0); // Doorbell 0 for Host Controller
    return 0;
}

static int xhci_controller_init(xhci_controller_t* xhci, pci_device_t* pci) {
    extern uint64_t g_hhdm_offset;
    xhci->bar = (pci->bar0 & ~0xF) + g_hhdm_offset;
    if ((pci->bar0 & 0x6) == 0x4) {
        xhci->bar += ((uint64_t)pci->bar1 << 32);
    }

    xhci->cap_length = *(volatile uint8_t*)(xhci->bar + XHCI_CAP_CAPLENGTH);
    xhci->op_base = xhci->bar + xhci->cap_length;
    
    uint32_t rt_offset = *(volatile uint32_t*)(xhci->bar + XHCI_CAP_RTS_OFFSET);
    xhci->rt_base = xhci->bar + rt_offset;

    uint32_t db_offset = *(volatile uint32_t*)(xhci->bar + XHCI_CAP_DB_OFFSET);
    xhci->db_base = xhci->bar + db_offset;

    uint32_t hcs1 = *(volatile uint32_t*)(xhci->bar + XHCI_CAP_HCSPARAMS1);
    xhci->max_slots = hcs1 & 0xFF;
    xhci->max_ports = (hcs1 >> 24) & 0xFF;

    kprintf("xHCI: BAR=%lx Slots=%d Ports=%d\n", xhci->bar, xhci->max_slots, xhci->max_ports);

    // 1. Reset Controller
    xhci_write_op32(xhci, XHCI_OP_USBCMD, 0); // Stop
    while (!(xhci_read_op32(xhci, XHCI_OP_USBSTS) & 1)); // Wait for HCHalted

    xhci_write_op32(xhci, XHCI_OP_USBCMD, (1 << 1)); // HCRST
    while (xhci_read_op32(xhci, XHCI_OP_USBCMD) & (1 << 1));

    kprintf("xHCI: Reset Successful\n");

    // 2. Setup Device Context Base Address Array Pointer (DCBAAP)
    xhci->dcbaap = (uint64_t*)kmalloc_raw_aligned(4096);
    memset(xhci->dcbaap, 0, 4096);
    *(volatile uint64_t*)(xhci->op_base + XHCI_OP_DCBAAP) = VIRT_TO_PHYS(xhci->dcbaap);

    // 3. Setup Command Ring (64 entries)
    xhci->cmd_ring = (xhci_trb_t*)kmalloc_raw_aligned(4096);
    memset(xhci->cmd_ring, 0, 4096);
    xhci->cmd_ring_index = 0;
    xhci->cmd_cycle = 1;

    // CRCR: Command Ring Control Register
    *(volatile uint64_t*)(xhci->op_base + XHCI_OP_CRCR) = VIRT_TO_PHYS(xhci->cmd_ring) | 1;

    // 4. Setup Event Ring (simplified: 1 segment)
    // Actually needs Event Ring Segment Table (ERST)
    void* erst = kmalloc_raw_aligned(4096);
    void* event_ring = kmalloc_raw_aligned(4096);
    memset(erst, 0, 4096);
    memset(event_ring, 0, 4096);

    xhci->event_ring = (xhci_trb_t*)event_ring;
    xhci->event_ring_index = 0;
    xhci->event_cycle = 1;

    // ERST Entry 0: Base Address, Size
    *(uint64_t*)erst = VIRT_TO_PHYS(event_ring);
    *(uint32_t*)((uint8_t*)erst + 8) = 256; // 256 entries

    // Runtime Registers: IMAN, IMOD, ERSTSZ, ERSTBA, ERDP
    // Interrupter 0
    *(volatile uint32_t*)(xhci->rt_base + 0x20 + 0x08) = 1; // ERSTSZ = 1
    *(volatile uint64_t*)(xhci->rt_base + 0x20 + 0x10) = VIRT_TO_PHYS(erst); // ERSTBA
    *(volatile uint64_t*)(xhci->rt_base + 0x20 + 0x18) = VIRT_TO_PHYS(event_ring) | (1 << 3); // ERDP + EHB

    // 5. Enable Slots
    uint32_t config = xhci_read_op32(xhci, XHCI_OP_CONFIG);
    config &= ~0xFF;
    config |= xhci->max_slots;
    xhci_write_op32(xhci, XHCI_OP_CONFIG, config);

    // 6. Start Controller
    xhci_write_op32(xhci, XHCI_OP_USBCMD, 1); // Run
    while (xhci_read_op32(xhci, XHCI_OP_USBSTS) & 1); // Wait for HCHalted to clear

    kprintf("xHCI: Controller Started\n");
    return 0;
}

static int xhci_poll_event(xhci_controller_t* xhci, xhci_trb_t* event) {
    // Note: event_ring is managed by Interrupter 0
    // Simplified: event_ring is at RT_BASE + 0x20
    xhci_trb_t* ev = &xhci->event_ring[xhci->event_ring_index];
    if ((ev->control & 1) != xhci->event_cycle) return -1;
    
    if (event) memcpy(event, ev, sizeof(xhci_trb_t));
    
    // Clear and Move (Simplified)
    // Move Head
    xhci->event_ring_index++;
    if (xhci->event_ring_index >= 256) {
        xhci->event_ring_index = 0;
        xhci->event_cycle ^= 1;
    }
    
    *(volatile uint64_t*)(xhci->rt_base + 0x20 + 0x18) = VIRT_TO_PHYS(&xhci->event_ring[xhci->event_ring_index]) | (1 << 3); // Dequeue pointer + EHB
    
    return 0;
}

static int xhci_enable_slot(xhci_controller_t* xhci) {
    xhci_trb_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.control = (TRB_TYPE_ENABLE_SLOT << 10);
    
    xhci_submit_cmd(xhci, &cmd);
    
    // Poll for Command Completion Event
    xhci_trb_t event;
    int timeout = 100000;
    while (xhci_poll_event(xhci, &event) == -1) {
        if (--timeout == 0) return -1;
    }
    
    uint32_t slot_id = (event.control >> 24) & 0xFF;
    return slot_id;
}

static int xhci_address_device(xhci_controller_t* xhci, uint32_t slot_id) {
    // 1. Setup Input Context (Simplified: just enough for Address Device)
    void* input_context = kmalloc_raw_aligned(4096);
    memset(input_context, 0, 4096);
    
    // 2. Setup Device Context for the slot
    void* device_context = kmalloc_raw_aligned(4096);
    memset(device_context, 0, 4096);
    xhci->dcbaap[slot_id] = VIRT_TO_PHYS(device_context);
    
    // 3. Submit Address Device Command
    xhci_trb_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.parameter = VIRT_TO_PHYS(input_context);
    cmd.control = (TRB_TYPE_ADDRESS_DEVICE << 10) | (slot_id << 24) | xhci->cmd_cycle;
    
    xhci_submit_cmd(xhci, &cmd);
    
    // Poll for completion
    xhci_trb_t event;
    int timeout = 100000;
    while (xhci_poll_event(xhci, &event) == -1) {
        if (--timeout == 0) return -1;
    }
    
    kprintf("xHCI: Device addressed on slot %d\n", slot_id);
    return 0;
}

void xhci_check_ports(xhci_controller_t* xhci) {
    for (int i = 1; i <= xhci->max_ports; i++) {
        uint32_t offset = 0x400 + (i - 1) * 0x10;
        uint32_t portsc = xhci_read_op32(xhci, offset);
        
        if (portsc & (1 << 0)) { // Current Connect Status
            kprintf("xHCI: Device detected on port %d (PORTSC=%x)\n", i, portsc);
            
            // Trigger Port Reset
            xhci_write_op32(xhci, offset, portsc | (1 << 4)); // Port Reset
            while (xhci_read_op32(xhci, offset) & (1 << 4));
            
            // Wait for port to become enabled
            while (!(xhci_read_op32(xhci, offset) & (1 << 1)));
            
            kprintf("xHCI: Port %d enabled, speed=%d. Enabling slot...\n", i, (portsc >> 10) & 0xF);
            
            int slot_id = xhci_enable_slot(xhci);
            if (slot_id > 0) {
                kprintf("xHCI: Slot %d enabled for port %d\n", slot_id, i);
                xhci_address_device(xhci, slot_id);
            }
        }
    }
}

void xhci_init(void) {
    pci_device_t* pci = 0;
    for (int i = 0; i < pci_get_device_count(); i++) {
        pci_device_t* dev = pci_get_device(i);
        if (dev->class_code == 0x0C && dev->subclass == 0x03 && dev->prog_if == 0x30) {
            pci = dev;
            break;
        }
    }

    if (!pci) {
        kprintf("xHCI: No controller found.\n");
        return;
    }

    kprintf("xHCI: Found controller at %d:%d:%d\n", pci->bus, pci->device, pci->function);
    if (xhci_controller_init(&g_xhci, pci) == 0) {
        xhci_check_ports(&g_xhci);
    }
}
