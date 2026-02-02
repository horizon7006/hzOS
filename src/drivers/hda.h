#ifndef HDA_H
#define HDA_H

#include <stdint.h>
#include <stddef.h>

/* Global Registers */
#define HDA_REG_GCAP        0x00 // Global Capabilities (2 bytes)
#define HDA_REG_VMIN        0x02 // Minor Version (1 byte)
#define HDA_REG_VMAJ        0x03 // Major Version (1 byte)
#define HDA_REG_OUTPAY      0x04 // Output Payload Capability (2 bytes)
#define HDA_REG_INPAY       0x06 // Input Payload Capability (2 bytes)
#define HDA_REG_GCTL        0x08 // Global Control (4 bytes)
#define HDA_REG_WAKEEN      0x0C // Wake Enable (2 bytes)
#define HDA_REG_STATESTS    0x0E // State Status (2 bytes)
#define HDA_REG_GSTS        0x10 // Global Status (2 bytes)
#define HDA_REG_INTCTL      0x20 // Interrupt Control (4 bytes)
#define HDA_REG_INTSTS      0x24 // Interrupt Status (4 bytes)
#define HDA_REG_WALCLK      0x30 // Wall Clock Counter (4 bytes)
#define HDA_REG_CORBLBASE   0x40 // CORB Lower Base Address (4 bytes)
#define HDA_REG_CORBUBASE   0x44 // CORB Upper Base Address (4 bytes)
#define HDA_REG_CORBWP      0x48 // CORB Write Pointer (2 bytes)
#define HDA_REG_CORBRP      0x4A // CORB Read Pointer (2 bytes)
#define HDA_REG_CORBCTL     0x4C // CORB Control (1 byte)
#define HDA_REG_CORBSTS     0x4D // CORB Status (1 byte)
#define HDA_REG_CORBSIZE    0x4E // CORB Size (1 byte)
#define HDA_REG_RIRBLBASE   0x50 // RIRB Lower Base Address (4 bytes)
#define HDA_REG_RIRBUBASE   0x54 // RIRB Upper Base Address (4 bytes)
#define HDA_REG_RIRBWP      0x58 // RIRB Write Pointer (2 bytes)
#define HDA_REG_RIRBCTL     0x5C // RIRB Control (1 byte)
#define HDA_REG_RIRBSTS     0x5D // RIRB Status (1 byte)
#define HDA_REG_RIRBSIZE    0x5E // RIRB Size (1 byte)
#define HDA_REG_IC          0x60 // Immediate Command (4 bytes)
#define HDA_REG_IR          0x64 // Immediate Response (4 bytes)
#define HDA_REG_ICS         0x68 // Immediate Command Status (2 bytes)

/* Stream Descriptor Registers (Base at 0x80) */
#define HDA_STREAM_CTRL     0x00 // Control (3 bytes)
#define HDA_STREAM_STS      0x03 // Status (1 byte)
#define HDA_STREAM_LPIB     0x04 // Link Position In Buffer (4 bytes)
#define HDA_STREAM_CBL      0x08 // Cyclic Buffer Length (4 bytes)
#define HDA_STREAM_LVI      0x0C // Last Valid Index (2 bytes)
#define HDA_STREAM_FIFOS    0x10 // FIFO Size (2 bytes)
#define HDA_STREAM_FMT      0x12 // Format (2 bytes)
#define HDA_STREAM_BDPL     0x18 // Buffer Descriptor List Lower (4 bytes)
#define HDA_STREAM_BDPU     0x1C // Buffer Descriptor List Upper (4 bytes)

typedef struct hda_controller {
    uintptr_t bar;
    uint32_t* corb;
    uint64_t* rirb;
    uint16_t corb_size_entries;
    uint16_t rirb_size_entries;
    uint16_t corb_wp;
    uint16_t rirb_rp;
    int num_iss; // Input Stream Slots
    int num_oss; // Output Stream Slots
    int num_bss; // Bidirectional Stream Slots
    
    // Found widgets
    int dac_node;
    int pin_node;
} hda_controller_t;

void hda_init(void);
uint64_t hda_send_verb(uint8_t codec_addr, uint8_t node_id, uint32_t verb);
void hda_play_sine(uint32_t freq_hz, uint32_t duration_ms);

#endif
