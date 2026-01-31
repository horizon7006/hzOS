#ifndef XHCI_H
#define XHCI_H

#include "../core/common.h"
#include "pci.h"

/* xHCI Capability Registers (Offsets from BAR0) */
#define XHCI_CAP_CAPLENGTH    0x00
#define XHCI_CAP_HCIVERSION   0x02
#define XHCI_CAP_HCSPARAMS1   0x04
#define XHCI_CAP_HCCPARAMS1   0x10
#define XHCI_CAP_DB_OFFSET    0x14
#define XHCI_CAP_RTS_OFFSET    0x18

/* xHCI Operational Registers (Offsets from BAR0 + CAPLENGTH) */
#define XHCI_OP_USBCMD        0x00
#define XHCI_OP_USBSTS        0x04
#define XHCI_OP_PAGESIZE      0x08
#define XHCI_OP_DNCTRL        0x14
#define XHCI_OP_CRCR          0x18
#define XHCI_OP_DCBAAP        0x30
#define XHCI_OP_CONFIG        0x38

/* xHCI TRB Types */
#define TRB_TYPE_NORMAL             1
#define TRB_TYPE_SETUP_STAGE        2
#define TRB_TYPE_DATA_STAGE         3
#define TRB_TYPE_STATUS_STAGE       4
#define TRB_TYPE_LINK               6
#define TRB_TYPE_ENABLE_SLOT        9
#define TRB_TYPE_ADDRESS_DEVICE    11
#define TRB_TYPE_PORT_STATUS_CHANGE 34

/* 16-byte Transfer Request Block (TRB) */
typedef struct {
    uint64_t parameter;
    uint32_t status;
    uint32_t control;
} xhci_trb_t;

/* xHCI Controller Structure */
typedef struct {
    uintptr_t bar;
    uintptr_t op_base;
    uintptr_t rt_base;
    uintptr_t db_base;

    uint8_t cap_length;
    uint16_t max_slots;
    uint16_t max_ports;

    xhci_trb_t* cmd_ring;
    uint32_t cmd_ring_index;
    uint8_t cmd_cycle;

    uint64_t* dcbaap;
} xhci_controller_t;

void xhci_init(void);
void xhci_check_ports(xhci_controller_t* xhci);

#endif
