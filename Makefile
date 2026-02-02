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
DIR_NET     := $(SRCDIR)/net

# Verbosity control
ifeq ($(V),1)
  Q :=
else
  Q := @
endif

INCLUDE_FLAGS := -I$(DIR_CORE) -I$(DIR_DRIVERS) -I$(DIR_FS) -I$(DIR_GUI) -I$(DIR_APPS) -I$(DIR_LIB) -I$(DIR_SHELL)

KERNEL_ELF := $(BUILDDIR)/kernel.elf
ISO        := $(BUILDDIR)/hzOS.iso

OBJS := \
  $(BUILDDIR)/kernel_entry.o \
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
  $(BUILDDIR)/blockdev.o \
  $(BUILDDIR)/gpt.o \
  $(BUILDDIR)/fat32.o \
  $(BUILDDIR)/exfat.o \
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
  $(BUILDDIR)/string.o \
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
  $(BUILDDIR)/hda.o \
  $(BUILDDIR)/acpi.o \
  $(BUILDDIR)/apic.o \
  $(BUILDDIR)/hpet.o \
  $(BUILDDIR)/smp.o \
  $(BUILDDIR)/smp_trampoline.o \
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

CFLAGS := -std=gnu11 -ffreestanding -O2 -Wall -Wextra -g -fno-pic -fno-pie -fno-stack-protector -mno-sse -mno-mmx -mno-80387 -mcmodel=kernel -mno-red-zone $(CFLAGS_ARCH) $(INCLUDE_FLAGS)
LDFLAGS := -nostdlib -T linker.ld -z max-page-size=0x1000 $(LDFLAGS_ARCH)

.PHONY: all kernel iso run-qemu debug-qemu clean

all: $(ISO)

kernel: $(KERNEL_ELF)

# Search paths for source files
vpath %.c $(DIR_CORE) $(DIR_DRIVERS) $(DIR_FS) $(DIR_GUI) $(DIR_APPS) $(DIR_LIB) $(DIR_SHELL) $(DIR_NET)
vpath %.s $(DIR_CORE) $(DIR_DRIVERS) $(DIR_FS) $(DIR_GUI) $(DIR_APPS) $(DIR_LIB) $(DIR_SHELL)
vpath %.S $(DIR_CORE)

$(BUILDDIR):
	$(Q)mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	@echo "  CC      $<"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.s | $(BUILDDIR)
	@echo "  AS      $<"
	$(Q)$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/%.o: %.S | $(BUILDDIR)
	@echo "  AS      $<"
	$(Q)$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/smp_trampoline.bin: src/core/smp_trampoline.S
	@echo "  NASM    $<"
	$(Q)nasm -f bin $< -o $@

$(BUILDDIR)/smp_trampoline.o: $(BUILDDIR)/smp_trampoline.bin
	@echo "  OBJCOPY $<"
	$(Q)objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

$(BUILDDIR)/sysroot: | $(BUILDDIR)
	$(Q)mkdir -p $@/bin

ext2.img: $(BUILDDIR)/sysroot | $(BUILDDIR)
	@echo "  GEN     $@"
	$(Q)mke2fs -d $(BUILDDIR)/sysroot -F $@ 2048 > /dev/null 2>&1

$(BUILDDIR)/ext2_img.o: ext2.img | $(BUILDDIR)
	@echo "  OBJCOPY $<"
	$(Q)objcopy -I binary -O elf64-x86-64 -B i386 ext2.img $@

$(KERNEL_ELF): $(OBJS) linker.ld | $(BUILDDIR)
	@echo "  LD      $@"
	$(Q)$(LD) $(LDFLAGS) -o $@ $(OBJS)

limine:
	$(Q)git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
	$(Q)$(MAKE) -C limine

iso: $(ISO)

$(ISO): $(KERNEL_ELF) limine.conf grub.cfg | $(BUILDDIR) limine
	@echo "  ISO     $@"
	$(Q)rm -rf $(ISOROOT)
	$(Q)mkdir -p $(ISOROOT)/boot
	$(Q)cp $(KERNEL_ELF) $(ISOROOT)/
	$(Q)cp $(KERNEL_ELF) $(ISOROOT)/boot/kernel.elf
	$(Q)cp limine.conf $(ISOROOT)/
	$(Q)cp limine.conf $(ISOROOT)/boot/
	$(Q)mkdir -p $(ISOROOT)/boot/limine
	$(Q)cp limine.conf $(ISOROOT)/boot/limine/
	$(Q)mkdir -p $(ISOROOT)/EFI/BOOT
	$(Q)cp limine.conf $(ISOROOT)/EFI/BOOT/
	$(Q)cp limine/limine-bios.sys $(ISOROOT)/boot/ 2>/dev/null || true
	$(Q)cp limine/limine-bios-cd.bin $(ISOROOT)/boot/ 2>/dev/null || true
	$(Q)cp limine/limine-uefi-cd.bin $(ISOROOT)/boot/ 2>/dev/null || true
	$(Q)cp limine/BOOTX64.EFI $(ISOROOT)/EFI/BOOT/ 2>/dev/null || true
	$(Q)cp limine/BOOTIA32.EFI $(ISOROOT)/EFI/BOOT/ 2>/dev/null || true
	$(Q)xorriso -as mkisofs -b boot/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISOROOT) -o $@
	$(Q)limine/limine bios-install $@

# Create a virtual hard disk image for AHCI testing
hdd.img:
	dd if=/dev/zero of=hdd.img bs=1M count=64

# Find OVMF if it exists
OVMF := /usr/share/ovmf/OVMF.fd
ifeq ($(wildcard $(OVMF)),)
  OVMF := /usr/share/OVMF/OVMF_CODE.fd
endif

run-qemu: $(ISO) hdd.img
	qemu-system-x86_64 -bios $(OVMF) -cdrom $(ISO) -drive if=none,id=dr1,file=hdd.img -device ich9-ahci,id=ahci -device ide-hd,drive=dr1,bus=ahci.0 -net nic,model=rtl8139 -net user,hostfwd=tcp::8080-:80,hostfwd=udp::8888-:8888 -smp 4 -device intel-hda -device hda-duplex,audiodev=audio0 -audiodev pa,id=audio0

debug-qemu: $(ISO) hdd.img
	qemu-system-x86_64 -bios $(OVMF) -cdrom $(ISO) -drive if=none,id=dr1,file=hdd.img -device ich9-ahci,id=ahci -device ide-hd,drive=dr1,bus=ahci.0 -net nic,model=rtl8139 -net user,hostfwd=tcp::8080-:80,hostfwd=udp::8888-:8888 -smp 4 -serial stdio -device intel-hda -device hda-duplex,audiodev=audio0 -audiodev pa,id=audio0

clean:
	rm -rf $(BUILDDIR)


