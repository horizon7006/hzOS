#include "rtl8139.h"
#include "pci.h"
#include "../core/io.h"
#include "../core/isr.h"
#include "../lib/memory.h"
#include "../lib/printf.h"

#include "../net/net.h"

static uint32_t io_base = 0;
static uint8_t mac_address[6];
static uint8_t* rx_buffer = 0;
static uint32_t rx_offset = 0;
static uint32_t current_tsad_index = 0;

static void rtl8139_handler(struct registers* regs) {
    UNUSED(regs);
    uint16_t status = inw(io_base + RTL_REG_ISR);
    outw(io_base + RTL_REG_ISR, status); 

    if (status & RTL_INT_ROK) {
        while ((inb(io_base + RTL_REG_CMD) & RTL_CMD_BUFE) == 0) {
            uint8_t* packet_ptr = rx_buffer + rx_offset;
            uint32_t header = *(uint32_t*)packet_ptr;
            uint16_t len = header >> 16;
            
            // Packet data starts after 4-byte header
            net_receive(packet_ptr + 4, len - 4); // len includes CRC (4 bytes)

            rx_offset = (rx_offset + len + 4 + 3) & ~3; // Align to 4 bytes
            rx_offset %= RX_BUF_SIZE;
            outw(io_base + RTL_REG_CAPR, rx_offset - 16);
        }
    }
}

void rtl8139_init(void) {
    kprintf("rtl8139: searching for device...\n");
    
    pci_device_t* dev = pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (!dev) {
        kprintf("rtl8139: device not found\n");
        return;
    }

    io_base = dev->bar0 & ~0x3;
    kprintf("rtl8139: found at IO base 0x%x\n", io_base);

    // Read IRQ Line
    uint8_t irq = pci_config_read_byte(dev->bus, dev->device, dev->function, 0x3C);
    kprintf("rtl8139: IRQ line %d\n", irq);

    // Enable PCI Bus Mastering
    uint16_t pci_command = pci_config_read_word(dev->bus, dev->device, dev->function, PCI_COMMAND);
    pci_command |= 0x04; // Bus Master Bit
    pci_config_write_word(dev->bus, dev->device, dev->function, PCI_COMMAND, pci_command);

    // Power on (Reset CONFIG1)
    outb(io_base + RTL_REG_CONFIG1, 0x00);

    // Soft Reset
    outb(io_base + RTL_REG_CMD, RTL_CMD_RESET);
    while ((inb(io_base + RTL_REG_CMD) & RTL_CMD_RESET) != 0);

    // Read MAC address
    for (int i = 0; i < 6; i++) {
        mac_address[i] = inb(io_base + RTL_REG_MAC0 + i);
    }
    kprintf("rtl8139: MAC %x:%x:%x:%x:%x:%x\n", 
            mac_address[0], mac_address[1], mac_address[2],
            mac_address[3], mac_address[4], mac_address[5]);

    // Initialize RX Buffer
    rx_buffer = (uint8_t*)kmalloc_a(RX_BUF_TOTAL);
    memset(rx_buffer, 0, RX_BUF_TOTAL);
    outl(io_base + RTL_REG_RBSTART, (uint32_t)VIRT_TO_PHYS(rx_buffer));

    // Initialize Interrupts
    irq_register_handler(irq, rtl8139_handler);
    outw(io_base + RTL_REG_IMR, RTL_INT_ROK | RTL_INT_TER | RTL_INT_RER | RTL_INT_TUV | RTL_INT_RXOVW | RTL_INT_PUN);

    // Receive Configuration
    // (Accept Physical Match, Multicast, Broadcast) + WRAP bit
    outl(io_base + RTL_REG_RCR, RTL_RCR_APM | RTL_RCR_AM | RTL_RCR_AB | RTL_RCR_WRAP);

    // Enable Transmitter and Receiver
    outb(io_base + RTL_REG_CMD, RTL_CMD_TE | RTL_CMD_RE);

    // Initialize Network Stack (Default QEMU IP 10.0.2.15)
    net_init(mac_address, (10 << 24) | (0 << 16) | (2 << 8) | 15);

    kprintf("rtl8139: initialized\n");
}

void rtl8139_send_packet(void* data, uint32_t len) {
    if (len > 1792) return; // Limit for RTL8139

    // Set TX Address
    outl(io_base + RTL_REG_TSAD0 + current_tsad_index * 4, (uint32_t)VIRT_TO_PHYS(data));
    
    // Set TX Status (this triggers the send)
    // Bit 0-12 is length, rest is flags (0x0000 means "start transmission")
    outl(io_base + RTL_REG_TSD0 + current_tsad_index * 4, len);

    current_tsad_index++;
    if (current_tsad_index > 3) current_tsad_index = 0;
}
