#include "hda.h"
#include "pci.h"
#include "../lib/printf.h"
#include "../lib/memory.h"
#include "../core/hpet.h"

static hda_controller_t g_hda;

static uint32_t hda_read32(hda_controller_t* hda, uint32_t reg) {
    return *(volatile uint32_t*)(hda->bar + reg);
}

static void hda_write32(hda_controller_t* hda, uint32_t reg, uint32_t val) {
    *(volatile uint32_t*)(hda->bar + reg) = val;
}

static uint16_t hda_read16(hda_controller_t* hda, uint32_t reg) {
    return *(volatile uint16_t*)(hda->bar + reg);
}

static void hda_write16(hda_controller_t* hda, uint32_t reg, uint16_t val) {
    *(volatile uint16_t*)(hda->bar + reg) = val;
}

static uint8_t hda_read8(hda_controller_t* hda, uint32_t reg) {
    return *(volatile uint8_t*)(hda->bar + reg);
}

static void hda_write8(hda_controller_t* hda, uint32_t reg, uint8_t val) {
    *(volatile uint8_t*)(hda->bar + reg) = val;
}

void hda_init(void) {
    pci_device_t* pci = NULL;
    for (int i = 0; i < pci_get_device_count(); i++) {
        pci_device_t* dev = pci_get_device(i);
        if (dev->class_code == PCI_CLASS_MULTIMEDIA && dev->subclass == PCI_SUBCLASS_HDA) {
            pci = dev;
            break;
        }
    }

    if (!pci) {
        kprintf("HDA: No controller found\n");
        return;
    }

    kprintf("HDA: Found controller at %02x:%02x.%x\n", pci->bus, pci->device, pci->function);

    extern uint64_t g_hhdm_offset;
    g_hda.bar = (pci->bar0 & ~0xF) + g_hhdm_offset;
    kprintf("HDA: MMIO at %lx\n", g_hda.bar);

    // 1. Reset the controller
    uint32_t gctl = hda_read32(&g_hda, HDA_REG_GCTL);
    hda_write32(&g_hda, HDA_REG_GCTL, gctl & ~1); // CRST = 0 (Reset)
    
    // Wait for CRST to become 0
    int timeout = 1000;
    while ((hda_read32(&g_hda, HDA_REG_GCTL) & 1) && timeout--) hpet_sleep(1);
    
    hpet_sleep(10);

    hda_write32(&g_hda, HDA_REG_GCTL, hda_read32(&g_hda, HDA_REG_GCTL) | 1); // CRST = 1 (Ready)
    
    // Wait for CRST to become 1
    timeout = 1000;
    while (!(hda_read32(&g_hda, HDA_REG_GCTL) & 1) && timeout--) hpet_sleep(1);

    if (!(hda_read32(&g_hda, HDA_REG_GCTL) & 1)) {
        kprintf("HDA: Controller reset timed out\n");
        return;
    }

    kprintf("HDA: Controller Reset Successful (Ver %d.%d)\n", 
            hda_read8(&g_hda, HDA_REG_VMAJ), hda_read8(&g_hda, HDA_REG_VMIN));

    // 2. Setup CORB/RIRB
    g_hda.corb = kmalloc_raw_aligned(1024); // 256 * 4 bytes
    g_hda.rirb = kmalloc_raw_aligned(2048); // 256 * 8 bytes
    g_hda.corb_size_entries = 256;
    g_hda.rirb_size_entries = 256;
    g_hda.corb_wp = 0;
    g_hda.rirb_rp = 0;

    // Stop rings before configuring
    hda_write8(&g_hda, HDA_REG_CORBCTL, 0);
    hda_write8(&g_hda, HDA_REG_RIRBCTL, 0);

    // Set Base Addresses
    hda_write32(&g_hda, HDA_REG_CORBLBASE, (uint32_t)VIRT_TO_PHYS(g_hda.corb));
    hda_write32(&g_hda, HDA_REG_CORBUBASE, (uint32_t)(VIRT_TO_PHYS(g_hda.corb) >> 32));
    hda_write32(&g_hda, HDA_REG_RIRBLBASE, (uint32_t)VIRT_TO_PHYS(g_hda.rirb));
    hda_write32(&g_hda, HDA_REG_RIRBUBASE, (uint32_t)(VIRT_TO_PHYS(g_hda.rirb) >> 32));

    // Set Write Pointer (CORB) and Read Pointer (RIRB) reset to 0
    hda_write16(&g_hda, HDA_REG_CORBWP, 0);
    // Read Pointer is set by Hardware, usually we write 1 to Reset bit in CORBRP if supported
    hda_write16(&g_hda, HDA_REG_CORBRP, 0x8000); // Bit 15 is CORBRP Reset
    timeout = 1000;
    while ((hda_read16(&g_hda, HDA_REG_CORBRP) & 0x8000) && timeout--) hpet_sleep(1);

    // Set RIRB WP Reset
    hda_write16(&g_hda, HDA_REG_RIRBWP, 0x8000);

    // Set Sizes (2 = 256 entries for both)
    hda_write8(&g_hda, HDA_REG_CORBSIZE, 2);
    hda_write8(&g_hda, HDA_REG_RIRBSIZE, 2);

    // Start Rings
    hda_write8(&g_hda, HDA_REG_CORBCTL, 0x02); // Enable DMA
    hda_write8(&g_hda, HDA_REG_RIRBCTL, 0x02); // Enable DMA

    uint16_t gcap = hda_read16(&g_hda, HDA_REG_GCAP);
    g_hda.num_iss = (gcap >> 8) & 0xF;
    g_hda.num_oss = (gcap >> 12) & 0xF;
    g_hda.num_bss = (gcap >> 3) & 0x1F;

    kprintf("HDA: ISS=%d, OSS=%d, BSS=%d\n", g_hda.num_iss, g_hda.num_oss, g_hda.num_bss);

    // 3. Enumerate Codecs
    uint16_t statests = hda_read16(&g_hda, HDA_REG_STATESTS);
    for (int i = 0; i < 15; i++) {
        if (statests & (1 << i)) {
            kprintf("HDA: Codec %d found\n", i);
            
            // Get Root Node Count
            uint64_t resp = hda_send_verb(i, 0, 0xF0004); // Get Parameter: Subordinate Node Count
            if (resp != 0xFFFFFFFFFFFFFFFFULL) {
                int start_node = (resp >> 16) & 0xFF;
                int num_nodes = resp & 0xFF;
                kprintf("HDA: Codec %d has %d nodes starting at %d\n", i, num_nodes, start_node);
                
                for (int n = start_node; n < start_node + num_nodes; n++) {
                    uint64_t caps = hda_send_verb(i, n, 0xF0009); // Audio Widget Capabilities
                    int type = (caps >> 20) & 0xF;
                    
                    if (type == 0x0) { // Audio Output
                        kprintf("HDA: Node %d is Audio Output\n", n);
                        if (g_hda.dac_node == 0) g_hda.dac_node = n;
                    } else if (type == 0x4) { // Pin Complex
                        kprintf("HDA: Node %d is Pin Complex\n", n);
                        if (g_hda.pin_node == 0) g_hda.pin_node = n;
                    }
                }
            }
        }
    }
}

uint64_t hda_send_verb(uint8_t codec_addr, uint8_t node_id, uint32_t verb) {
    uint32_t full_verb = (codec_addr << 28) | (node_id << 20) | (verb & 0xFFFFF);
    
    uint16_t next_wp = (g_hda.corb_wp + 1) % g_hda.corb_size_entries;
    
    // 1. Wait for space in CORB
    int timeout = 10000;
    while (next_wp == (hda_read16(&g_hda, HDA_REG_CORBRP) & 0xFF) && timeout--) hpet_sleep(1);
    if (timeout < 0) {
        kprintf("HDA: CORB stall\n");
        return 0xFFFFFFFFFFFFFFFFULL;
    }

    // 2. Submit to CORB
    g_hda.corb[next_wp] = full_verb;
    g_hda.corb_wp = next_wp;
    hda_write16(&g_hda, HDA_REG_CORBWP, next_wp);

    // 3. Wait for response in RIRB
    timeout = 10000;
    while (timeout--) {
        uint16_t rirb_wp = hda_read16(&g_hda, HDA_REG_RIRBWP) & 0xFF;
        if (rirb_wp != g_hda.rirb_rp) {
            g_hda.rirb_rp = (g_hda.rirb_rp + 1) % g_hda.rirb_size_entries;
            return g_hda.rirb[g_hda.rirb_rp];
        }
        hpet_sleep(1);
    }

    kprintf("HDA: Verb timeout (Codec %d, Node %d)\n", codec_addr, node_id);
    return 0xFFFFFFFFFFFFFFFFULL;
}

void hda_play_sine(uint32_t freq_hz, uint32_t duration_ms) {
    if (g_hda.dac_node == 0 || g_hda.pin_node == 0) return;

    uint8_t codec = 0; // Assume codec 0
    uint16_t format = 0x4011; // 44.1kHz, 16-bit, 2 channels

    // 1. Setup Path
    hda_send_verb(codec, g_hda.pin_node, 0x70740); // Pin Ctrl: Out Enable
    hda_send_verb(codec, g_hda.dac_node, 0x70610); // Stream 1, Channel 0
    hda_send_verb(codec, g_hda.pin_node, 0x70610); // Stream 1, Channel 0
    hda_send_verb(codec, g_hda.dac_node, 0x24011); // Set Format

    // 2. Setup DMA Stream (OSS0)
    uint32_t stream_off = 0x80 + (g_hda.num_iss * 0x20);
    
    // Stop stream
    hda_write8(&g_hda, stream_off + HDA_STREAM_CTRL, 0);

    // Allocate Buffer (1 second of stereo 16-bit 44.1kHz = 176400 bytes)
    uint32_t buf_size = 176400;
    int16_t* pcm = kmalloc_raw_aligned(buf_size);
    
    // Fill with sine wave
    for (uint32_t i = 0; i < buf_size / 4; i++) {
        // Simplified: using a fixed step for sine instead of math.h for now
        // This is just a placeholder implementation until we have math.h
        pcm[i*2] = (i % (44100/freq_hz) < (22050/freq_hz)) ? 10000 : -10000; // Square wave as proxy
        pcm[i*2 + 1] = pcm[i*2];
    }

    // Setup BDL (Buffer Descriptor List)
    uint64_t* bdl = kmalloc_raw_aligned(128);
    bdl[0] = VIRT_TO_PHYS(pcm); // Address
    bdl[1] = 0; // High Address (zero for now as pcm is in lower 4GB)
    bdl[2] = buf_size & 0xFFFFFFFF; // Length
    bdl[3] = 0x01;           // IOC = 1

    hda_write32(&g_hda, stream_off + HDA_STREAM_BDPL, (uint32_t)VIRT_TO_PHYS(bdl));
    hda_write32(&g_hda, stream_off + HDA_STREAM_BDPU, (uint32_t)(VIRT_TO_PHYS(bdl) >> 32));
    hda_write32(&g_hda, stream_off + HDA_STREAM_CBL, buf_size);
    hda_write16(&g_hda, stream_off + HDA_STREAM_LVI, 0); // 1 fragment
    hda_write16(&g_hda, stream_off + HDA_STREAM_FMT, format);
    hda_write8(&g_hda, stream_off + HDA_STREAM_CTRL + 2, 0x10); // Stream ID 1

    // Start stream
    hda_write8(&g_hda, stream_off + HDA_STREAM_CTRL, hda_read8(&g_hda, stream_off + HDA_STREAM_CTRL) | 2); // Run

    kprintf("HDA: Playing tone...\n");
    hpet_sleep(duration_ms);
    
    // Stop stream
    hda_write8(&g_hda, stream_off + HDA_STREAM_CTRL, 0);
}
