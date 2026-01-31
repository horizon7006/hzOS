# TOOLCHAIN selection:
#   Native 32-bit: make TOOLCHAIN=native    (default)
#   Cross i686-elf: make TOOLCHAIN=cross

TOOLCHAIN ?= native

SRCDIR   := src
BUILDDIR := build
ISOROOT  := iso

# Subdirectories
DIR_CORE    := $(SRCDIR)/core
DIR_DRIVERS := $(SRCDIR)/drivers
DIR_FS      := $(SRCDIR)/fs
DIR_GUI     := $(SRCDIR)/gui
DIR_APPS    := $(SRCDIR)/apps
DIR_LIB     := $(SRCDIR)/lib
DIR_SHELL   := $(SRCDIR)/shell

INCLUDE_FLAGS := -I$(DIR_CORE) -I$(DIR_DRIVERS) -I$(DIR_FS) -I$(DIR_GUI) -I$(DIR_APPS) -I$(DIR_LIB) -I$(DIR_SHELL)

KERNEL_ELF := $(BUILDDIR)/kernel.elf
ISO        := $(BUILDDIR)/hzOS.iso

OBJS := \
  $(BUILDDIR)/kernel_entry.o \
  $(BUILDDIR)/boot_64.o \
  $(BUILDDIR)/kernel.o \
  $(BUILDDIR)/gdt.o \
  $(BUILDDIR)/gdt_flush.o \
  $(BUILDDIR)/idt.o \
  $(BUILDDIR)/isr.o \
  $(BUILDDIR)/isr_stub.o \
  $(BUILDDIR)/serial.o \
  $(BUILDDIR)/keyboard.o \
  $(BUILDDIR)/mouse.o \
  $(BUILDDIR)/vesa.o \
  $(BUILDDIR)/rtc.o \
  $(BUILDDIR)/ramdisk.o \
  $(BUILDDIR)/filesystem.o \
  $(BUILDDIR)/ext2.o \
  $(BUILDDIR)/ext2_img.o \
  $(BUILDDIR)/terminal.o \
  $(BUILDDIR)/command.o \
  $(BUILDDIR)/hzsh_config.o \
  $(BUILDDIR)/printf.o \
  $(BUILDDIR)/hzlib.o \
  $(BUILDDIR)/memory.o \
  $(BUILDDIR)/font.o \
  $(BUILDDIR)/graphics.o \
  $(BUILDDIR)/window_manager.o \
  $(BUILDDIR)/gui.o \
  $(BUILDDIR)/file_browser.o \
  $(BUILDDIR)/image_viewer.o \
  $(BUILDDIR)/notepad.o \
  $(BUILDDIR)/terminal_app.o \
  $(BUILDDIR)/about.o \
  $(BUILDDIR)/upng.o \
  $(BUILDDIR)/pci.o \
  $(BUILDDIR)/ahci.o \
  $(BUILDDIR)/nvme.o \
  $(BUILDDIR)/xhci.o \
  $(BUILDDIR)/elf.o \
  $(BUILDDIR)/process.o \
  $(BUILDDIR)/syscall.o \
  $(BUILDDIR)/rtl8139.o \
  $(BUILDDIR)/net.o \
  $(BUILDDIR)/icmp.o \
  $(BUILDDIR)/udp.o \
  $(BUILDDIR)/tcp.o \
  $(BUILDDIR)/paging.o \
  $(BUILDDIR)/timer.o

# Toolchain configuration
ifeq ($(TOOLCHAIN),cross)
  CC := i686-elf-gcc
  LD := i686-elf-ld
  CFLAGS_ARCH :=
  LDFLAGS_ARCH :=
else
  CC := gcc
  LD := ld
  CFLAGS_ARCH := -m64 -mcmodel=kernel -mno-red-zone
  LDFLAGS_ARCH := -m elf_x86_64 -z max-page-size=0x1000
endif

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -g -fno-pic -fno-pie -fno-stack-protector -mno-sse -mno-mmx -mno-80387 $(CFLAGS_ARCH) $(INCLUDE_FLAGS)
LDFLAGS := -nostdlib -T linker.ld $(LDFLAGS_ARCH)

.PHONY: all kernel iso run-qemu debug-qemu clean

all: $(ISO)

kernel: $(KERNEL_ELF)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/kernel.o: $(DIR_CORE)/kernel.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/kernel_entry.o: $(DIR_CORE)/kernel_entry.s | $(BUILDDIR)
	$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/boot_64.o: $(DIR_CORE)/boot_64.s | $(BUILDDIR)
	$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/gdt.o: $(DIR_CORE)/gdt.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/gdt_flush.o: $(DIR_CORE)/gdt_flush.s | $(BUILDDIR)
	$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/idt.o: $(DIR_CORE)/idt.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/isr.o: $(DIR_CORE)/isr.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/isr_stub.o: $(DIR_CORE)/isr_stub.s | $(BUILDDIR)
	$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/serial.o: $(DIR_DRIVERS)/serial.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/keyboard.o: $(DIR_DRIVERS)/keyboard.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/mouse.o: $(DIR_DRIVERS)/mouse.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/vesa.o: $(DIR_DRIVERS)/vesa.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/rtc.o: $(DIR_DRIVERS)/rtc.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/ramdisk.o: $(DIR_DRIVERS)/ramdisk.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/rtl8139.o: $(DIR_DRIVERS)/rtl8139.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/nvme.o: $(DIR_DRIVERS)/nvme.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/xhci.o: $(DIR_DRIVERS)/xhci.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/net.o: src/net/net.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/icmp.o: src/net/icmp.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/udp.o: src/net/udp.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/tcp.o: src/net/tcp.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/filesystem.o: $(DIR_FS)/filesystem.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/ext2.o: $(DIR_FS)/ext2.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create a tiny ext2 image on the host (Linux/WSL) with sysroot content.
# User binary building commented out - using integrated apps instead
#$(BUILDDIR)/sysroot: | $(BUILDDIR) user-bin
#	mkdir -p $@/bin
#	cp user/hello.prog $@/bin/
#	cp user/about.prog $@/bin/
#	cp user/notepad.prog $@/bin/
#	cp user/terminal.prog $@/bin/

# Empty sysroot for now since we're not using user binaries
$(BUILDDIR)/sysroot: | $(BUILDDIR)
	mkdir -p $@/bin

ext2.img: $(BUILDDIR)/sysroot | $(BUILDDIR)
	mke2fs -d $(BUILDDIR)/sysroot -F $@ 2048

#user-bin:
#	$(MAKE) -C user

$(BUILDDIR)/ext2_img.o: ext2.img | $(BUILDDIR)
	objcopy -I binary -O elf64-x86-64 -B i386 ext2.img $@

$(BUILDDIR)/terminal.o: $(DIR_SHELL)/terminal.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/command.o: $(DIR_SHELL)/command.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/hzsh_config.o: $(DIR_SHELL)/hzsh_config.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/printf.o: $(DIR_LIB)/printf.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/hzlib.o: $(DIR_LIB)/hzlib.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/memory.o: $(DIR_LIB)/memory.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/font.o: $(DIR_GUI)/font.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/pci.o: $(DIR_DRIVERS)/pci.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/ahci.o: $(DIR_DRIVERS)/ahci.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/elf.o: $(DIR_CORE)/elf.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/process.o: $(DIR_CORE)/process.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/syscall.o: $(DIR_CORE)/syscall.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/paging.o: $(DIR_CORE)/paging.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/timer.o: $(DIR_CORE)/timer.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/graphics.o: $(DIR_GUI)/graphics.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/window_manager.o: $(DIR_GUI)/window_manager.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/gui.o: $(DIR_GUI)/gui.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/file_browser.o: $(DIR_APPS)/file_browser.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/image_viewer.o: $(DIR_APPS)/image_viewer.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/notepad.o: $(DIR_APPS)/notepad.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/terminal_app.o: $(DIR_APPS)/terminal_app.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/about.o: $(DIR_APPS)/about.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILDDIR)/upng.o: $(DIR_LIB)/upng.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJS) linker.ld | $(BUILDDIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: $(ISO)

$(ISO): $(KERNEL_ELF) grub.cfg | $(BUILDDIR)
	mkdir -p $(ISOROOT)/boot/grub
	cp grub.cfg $(ISOROOT)/boot/grub/grub.cfg
	cp $(KERNEL_ELF) $(ISOROOT)/boot/kernel.elf
	grub-mkrescue -o $@ $(ISOROOT)

# Create a virtual hard disk image for AHCI testing
hdd.img:
	dd if=/dev/zero of=hdd.img bs=1M count=64

run-qemu: $(ISO) hdd.img
	qemu-system-x86_64 -cdrom $(ISO) -drive if=none,id=dr1,file=hdd.img -device ich9-ahci,id=ahci -device ide-hd,drive=dr1,bus=ahci.0 -net nic,model=rtl8139 -net user,hostfwd=tcp::8080-:80,hostfwd=udp::8888-:8888

debug-qemu: $(ISO) hdd.img
	qemu-system-i386 -cdrom $(ISO) -drive if=none,id=dr1,file=hdd.img -device ich9-ahci,id=ahci -device ide-hd,drive=dr1,bus=ahci.0 -net nic,model=rtl8139 -net user -serial stdio

clean:
	rm -rf $(BUILDDIR)