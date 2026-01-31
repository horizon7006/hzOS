#include "command.h"
#include "terminal.h"
#include "printf.h"
#include "filesystem.h"
#include "ext2.h"
#include "gui.h"
#include "../core/io.h"
#include "../core/elf.h"
#include "../core/process.h"
#include "../drivers/ahci.h"
#include "../net/net.h"

typedef struct {
    const char* name;
    const char* help;
    command_func_t fn;
} command_entry_t;

static void cmd_help(const char* args);
static void cmd_clear(const char* args);
static void cmd_about(const char* args);
static void cmd_lf(const char* args);
static void cmd_cat(const char* args);
static void cmd_pwd(const char* args);
static void cmd_cd(const char* args);
static void cmd_ext2_mount(const char* args);
static void cmd_ext2_info(const char* args);
static void cmd_ext2_lsroot(const char* args);
static void cmd_gui(const char* args);
static void cmd_reboot(const char* args);
static void cmd_shutdown(const char* args);
static void cmd_osfetch(const char* args);
static void cmd_mkdir(const char* args);
static void cmd_touch(const char* args);
static void cmd_write(const char* args);
static void cmd_files(const char* args);
static void cmd_makesamplepng(const char* args);
static void cmd_lspci(const char* args);
static void cmd_echo(const char* args);
static void cmd_exec(const char* args);
static void cmd_disktest(const char* args);
static void cmd_diskinfo(const char* args);
static void cmd_netinfo(const char* args);
static void cmd_ping(const char* args);

static command_entry_t commands[] = {
    { "help",        "Show available commands",       cmd_help        },
    { "clear",       "Clear the screen",              cmd_clear       },
    { "about",       "Show information about hzOS",   cmd_about       },
    { "osfetch",     "Show system information",       cmd_osfetch     },
    { "lf",          "List directory contents",       cmd_lf          },
    { "echo",        "Echo arguments",                cmd_echo        },
    { "cat",         "Print file contents",           cmd_cat         },
    { "pwd",         "Print current directory",       cmd_pwd         },
    { "cd",          "Change current directory",      cmd_cd          },
    { "mkdir",       "Create a directory",            cmd_mkdir       },
    { "touch",       "Create an empty file",          cmd_touch       },
    { "write",       "Write text to file",            cmd_write       },
    { "ext2mount",  "Mount ext2 from ramdisk",       cmd_ext2_mount  },
    { "ext2info",   "Show ext2 superblock summary",  cmd_ext2_info   },
    { "ext2lsroot", "List root dir of ext2 volume",  cmd_ext2_lsroot },
    { "gui",        "Start simple text-mode GUI",    cmd_gui        },
    { "files",      "Start File Browser",            cmd_files      },
    { "lspci",      "List PCI devices",              cmd_lspci      },
    { "reboot",     "Reboot the system",             cmd_reboot      },
    { "shutdown",   "Shutdown the system",           cmd_shutdown    },
    { "makesamplepng", "Make sample PNG",           cmd_makesamplepng },
    { "exec",       "Execute ELF binary",            cmd_exec       },
    { "disktest",   "Test AHCI disk read (LBA 0)",   cmd_disktest   },
    { "diskinfo",   "Show AHCI disk identifying info", cmd_diskinfo   },
    { "netinfo",    "Show network interface info",   cmd_netinfo    },
    { "ping",       "Send ARP request to test reachability", cmd_ping },
};

static const size_t command_count = sizeof(commands) / sizeof(commands[0]);

void command_init(void) {
    kprintf("commands: %u registered\n", (unsigned)command_count);
}

static void skip_spaces(const char** s) {
    while (**s == ' ' || **s == '\t') {
        (*s)++;
    }
}

static int str_eq(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

void command_execute(const char* line) {
    if (!line) return;

    const char* p = line;
    skip_spaces(&p);

    if (*p == '\0') return;

    const char* name_start = p;
    while (*p && *p != ' ' && *p != '\t') {
        p++;
    }

    size_t name_len = (size_t)(p - name_start);

    skip_spaces(&p);
    const char* args = p;

    for (size_t i = 0; i < command_count; i++) {
        const char* cname = commands[i].name;
        size_t j = 0;
        while (j < name_len && cname[j] && cname[j] == name_start[j]) {
            j++;
        }
        if (j == name_len && cname[j] == '\0') {
            commands[i].fn(args);
            return;
        }
    }

    kprintf("Unknown command: %s\n", line);
}

static void cmd_help(const char* args) {
    (void)args;
    terminal_writeln("Available commands:");
    for (size_t i = 0; i < command_count; i++) {
        kprintf("  %s - %s\n", commands[i].name, commands[i].help);
    }
}

static void cmd_clear(const char* args) {
    (void)args;
    terminal_clear();
}

static void cmd_about(const char* args) {
    (void)args;
    terminal_writeln("hzOS: tiny hobby kernel with a toy shell and in-memory FS.");
    terminal_writeln("Build info: simple 32-bit x86, BIOS, Multiboot.");
}

static void cmd_osfetch(const char* args) {
    (void)args;
    
    // ASCII Logo and Info
    // Design:
    //   +------+  OS:      hzOS
    //   |  hz  |  Version: ALPHA A.5
    //   +------+  Kernel:  hzKernel
    //             Arch:    i386

    kprintf("  +------+  OS:      hzOS\n");
    kprintf("  |  hz  |  Version: ALPHA A.5\n");
    kprintf("  +------+  Kernel:  hzKernel\n");
    kprintf("            Arch:    i386\n");
    kprintf("\n");
}

static void cmd_lf(const char* args) {
    const char* path = args;
    if (!path) {
        fs_list("/");
        return;
    }

    while (*path == ' ' || *path == '\t') {
        path++;
    }

    if (*path == '\0') {
        fs_list("/");
    } else {
        fs_list(path);
    }
}

static void cmd_echo(const char* args) {
    (void)args;
    terminal_writeln(args);
}   

static void cmd_cat(const char* args) {
    const char* path = args;
    if (!path) {
        kprintf("cat: path required\n");
        return;
    }

    while (*path == ' ' || *path == '\t') {
        path++;
    }

    if (*path == '\0') {
        kprintf("cat: path required\n");
        return;
    }

    size_t len = 0;
    const char* data = fs_read(path, &len);
    if (!data) {
        kprintf("cat: cannot open %s\n", path);
        return;
    }

    for (size_t i = 0; i < len; i++) {
        terminal_putc(data[i]);
    }
    terminal_putc('\n');
}

static void cmd_pwd(const char* args) {
    (void)args;
    kprintf("%s\n", fs_get_cwd());
}

static void cmd_cd(const char* args) {
    const char* path = args;
    if (!path) {
        kprintf("cd: path required\n");
        return;
    }

    while (*path == ' ' || *path == '\t') {
        path++;
    }

    if (*path == '\0') {
        kprintf("cd: path required\n");
        return;
    }

    if (fs_chdir(path) != 0) {
        kprintf("cd: cannot cd to %s\n", path);
        return;
    }

    kprintf("cwd: %s\n", fs_get_cwd());
}

static void cmd_ext2_mount(const char* args) {
    (void)args;
    if (ext2_mount_from_ramdisk() != 0) {
        kprintf("ext2mount: failed (no ramdisk or not ext2)\n");
    }
}

static void cmd_ext2_info(const char* args) {
    (void)args;
    ext2_print_super();
}

static void cmd_ext2_lsroot(const char* args) {
    (void)args;
    ext2_list_root();
}

static void cmd_gui(const char* args) {
    (void)args;
    gui_start();
}

static void cmd_reboot(const char* args) {
    (void)args;
    terminal_writeln("Rebooting...");
    
    // Pulse CPU Reset line via 8042 Keyboard Controller
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);
    
    // Halt if fail
    asm volatile("hlt");
}

static void cmd_shutdown(const char* args) {
    (void)args;
    terminal_writeln("Shutting down...");
    
    // Disable interrupts
    asm volatile("cli");
    
    // Method 1: ACPI Shutdown (works on real hardware and many emulators)
    // This is the proper way to shutdown via ACPI
    outw(0x604, 0x2000);  // QEMU
    outw(0xB004, 0x2000); // Bochs
    
    // Method 2: VirtualBox shutdown
    outw(0x4004, 0x3400);
    
    // Method 3: Try ACPI SLP_TYPa/SLP_TYPb (typical ACPI PM1a register)
    // This attempts to use ACPI properly - on typical systems PM1a_CNT is at 0x404 or 0x1004
    outw(0x1004, 0x2000); // PM1a_CNT with SLP_TYPa=5, SLP_EN=1 (typical for S5/shutdown)
    outw(0x404, 0x2000);
    
    // Method 4: APM Shutdown (older method)
    // Connect to APM
    outw(0x5307, 0x0001); // Connect APM interface
    outw(0x5308, 0x0001); // Set APM version
    outw(0x5307, 0x0003); // Shutdown
    
    // If we're still here, none of the above worked
    terminal_writeln("It is now safe to turn off your computer.");
    terminal_writeln("(Manual power off required)");
    
    // Infinite halt loop
    while(1) asm volatile("hlt");
}


static void cmd_mkdir(const char* args) {
    if (!args || !*args) {
        kprintf("mkdir: usage: mkdir <path>\n");
        return;
    }
    const char* p = args; 
    while (*p == ' ') p++;
    if (!*p) { kprintf("mkdir: path required\n"); return; }
    
    if (fs_mkdir(p) != 0) {
        kprintf("mkdir: cannot create directory '%s'\n", p);
    }
}

static void cmd_touch(const char* args) {
    if (!args || !*args) {
        kprintf("touch: usage: touch <path>\n");
        return;
    }
    const char* p = args; 
    while (*p == ' ') p++;
    if (!*p) { kprintf("touch: path required\n"); return; }

    if (fs_mkfile(p) != 0) {
        kprintf("touch: cannot create file '%s'\n", p);
    }
}

static void cmd_write(const char* args) {
    if (!args || !*args) {
        kprintf("write: usage: write <path> <content>\n");
        return;
    }
    
    // Parse path
    const char* p = args;
    while (*p == ' ') p++;
    if (!*p) { kprintf("write: path required\n"); return; }
    
    const char* path_start = p;
    while (*p && *p != ' ') p++;
    
    size_t path_len = (size_t)(p - path_start);
    if (path_len == 0) { kprintf("write: path required\n"); return; }
    
    char path_buf[256];
    if (path_len > 255) path_len = 255;
    for (size_t i=0; i<path_len; i++) path_buf[i] = path_start[i];
    path_buf[path_len] = 0;
    
    // Parse content
    while (*p == ' ') p++;
    const char* content = p;
    
    size_t content_len = 0; 
    while (content[content_len]) content_len++;
    
    if (fs_write_file(path_buf, content, content_len) != 0) {
        kprintf("write: failed to write to '%s'\n", path_buf);
    } else {
        kprintf("Written %u bytes to '%s'\n", (unsigned)content_len, path_buf);
    }
}

#include "../apps/file_browser.h"
static void cmd_files(const char* args) {
    // Default to root or provided path
    const char* path = "/";
    if (args && *args) {
        while (*args == ' ') args++;
        if (*args) path = args;
    }
    file_browser_create(path);
}

static void cmd_makesamplepng(const char* args) {
    (void)args;
    // 1x1 Red Pixel PNG
    unsigned char png_data[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // Signature
        0x00, 0x00, 0x00, 0x0D, // IHDR Length
        0x49, 0x48, 0x44, 0x52, // IHDR
        0x00, 0x00, 0x00, 0x01, // W=1
        0x00, 0x00, 0x00, 0x01, // H=1
        0x08, 0x02, 0x00, 0x00, 0x00, // Depth=8, Color=2 (RGB)
        0x90, 0x77, 0x53, 0xDE, // CRC
        0x00, 0x00, 0x00, 0x0C, // IDAT Length
        0x49, 0x44, 0x41, 0x54, // IDAT
        0x08, 0xD7, 0x63, 0xF8, 0xCF, 0xC0, 0x00, 0x00, // Data
        0x03, 0x01, 0x01, 0x00, 
        0x18, 0xDD, 0x8D, 0xB0, // CRC
        0x00, 0x00, 0x00, 0x00, // IEND Length
        0x49, 0x45, 0x4E, 0x44, // IEND
        0xAE, 0x42, 0x60, 0x82  // CRC
    };
    
    if (fs_write_file("/sample.png", (char*)png_data, sizeof(png_data)) == 0) {
        kprintf("Created /sample.png (1x1 Red Pixel)\n");
    } else {
        kprintf("Failed to create /sample.png\n");
    }
}

#include "../drivers/pci.h"
static void cmd_lspci(const char* args) {
    (void)args;
    
    int count = pci_get_device_count();
    kprintf("PCI Devices (%d found):\n", count);
    kprintf("Bus:Dev.Fn  Vendor:Device  Class/Sub/IF\n");
    kprintf("------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        pci_device_t* dev = pci_get_device(i);
        if (!dev) continue;
        
        kprintf("%02x:%02x.%x     %04x:%04x      %02x/%02x/%02x\n",
            dev->bus, dev->device, dev->function,
            dev->vendor_id, dev->device_id,
            dev->class_code, dev->subclass, dev->prog_if);
    }
}

static void cmd_exec(const char* args) {
    const char* path = args;
    if (!path || !*path) {
        kprintf("exec: usage: exec <path>\n");
        return;
    }
    
    // Skip whitespace
    while (*path == ' ' || *path == '\t') path++;
    
    if (*path == '\0') {
        kprintf("exec: usage: exec <path>\n");
        return;
    }
    
    kprintf("exec: loading %s\n", path);
    
    // Read file from EXT2 or VFS
    size_t size = 0;
    const char* data = fs_read(path, &size);
    if (!data) {
        kprintf("exec: cannot read %s\n", path);
        return;
    }
    
    kprintf("exec: loaded %d bytes\n", (int)size);
    
    // Validate ELF
    if (elf_validate(data) != 0) {
        kprintf("exec: not a valid ELF binary\n");
        return;
    }
    
    // Load ELF into memory
    if (elf_load(data, size) != 0) {
        kprintf("exec: failed to load ELF\n");
        return;
    }
    
    // Get entry point
    uint32_t entry = elf_get_entry(data);
    
    // Create process
    process_t* proc = process_create(path, entry);
    if (!proc) {
        kprintf("exec: failed to create process\n");
        return;
    }
    
    // Execute
    process_execute(proc);
}

static void cmd_disktest(const char* args) {
    (void)args;
    int port_count = ahci_get_port_count();
    if (port_count == 0) {
        kprintf("disktest: no SATA drives detected\n");
        return;
    }

    HBA_PORT* port = ahci_get_port(0);
    kprintf("disktest: reading LBA 0 from port %d...\n", 0);

    // Buffer for one sector (512 bytes = 256 words)
    // Needs to be kmalloc'd if we want it to be safely aligned/accessible by AHCI DMA
    // (though for 512 bytes it might be okay on stack, best to be safe)
    static uint16_t sector_buf[256];
    memset(sector_buf, 0, 512);

    if (ahci_read(port, 0, 0, 1, sector_buf) == 0) {
        kprintf("disktest: read successful!\n");
        kprintf("Data (first 32 bytes):\n");
        uint8_t* ptr = (uint8_t*)sector_buf;
        for (int i = 0; i < 32; i++) {
            kprintf("%02x ", ptr[i]);
            if ((i + 1) % 16 == 0) kprintf("\n");
        }
    } else {
        kprintf("disktest: read failed\n");
    }
}

static void cmd_diskinfo(const char* args) {
    (void)args;
    int port_count = ahci_get_port_count();
    if (port_count == 0) {
        kprintf("diskinfo: no SATA drives detected\n");
        return;
    }

    HBA_PORT* port = ahci_get_port(0);
    static uint16_t info_buf[256];
    memset(info_buf, 0, 512);

    if (ahci_identify(port, info_buf) == 0) {
        kprintf("diskinfo: identification successful!\n");
        
        // Model number is at words 27-46
        char model[41];
        for (int i = 0; i < 20; i++) {
            uint16_t w = info_buf[27 + i];
            model[i*2] = (char)(w >> 8);
            model[i*2 + 1] = (char)(w & 0xFF);
        }
        model[40] = '\0';
        kprintf("Model: %s\n", model);

        // LBA48 support at word 83 bit 10
        if (info_buf[83] & (1 << 10)) {
            kprintf("Support: LBA48\n");
            // Capacity at words 100-103
            uint32_t sectors_low = info_buf[100] | ((uint32_t)info_buf[101] << 16);
            kprintf("Capacity (sectors): %d\n", sectors_low);
        } else {
            kprintf("Support: LBA28 only\n");
        }
    } else {
        kprintf("diskinfo: identification failed\n");
    }
}

static void cmd_netinfo(const char* args) {
    (void)args;
    uint8_t mac[6];
    net_get_mac(mac);
    uint32_t ip = net_get_ip();
    
    kprintf("Network Interface Info:\n");
    kprintf("  MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    kprintf("  IPv4 Address: %d.%d.%d.%d\n",
           (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

static void cmd_ping(const char* args) {
    if (!args || !*args) {
        kprintf("Usage: ping <ip>\n");
        return;
    }
    
    // Simple IP parser "a.b.c.d"
    int a, b, c, d;
    const char* p = args;
    a = 0; while (*p >= '0' && *p <= '9') a = a * 10 + (*p++ - '0'); if (*p == '.') p++;
    b = 0; while (*p >= '0' && *p <= '9') b = b * 10 + (*p++ - '0'); if (*p == '.') p++;
    c = 0; while (*p >= '0' && *p <= '9') c = c * 10 + (*p++ - '0'); if (*p == '.') p++;
    d = 0; while (*p >= '0' && *p <= '9') d = d * 10 + (*p++ - '0');
    
    uint32_t target_ip = htonl((a << 24) | (b << 16) | (c << 8) | d);
    kprintf("ARP PING %d.%d.%d.%d...\n", a, b, c, d);
    arp_send_request(target_ip);
}

