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
  $(BUILDDIR)/printf.o \
  $(BUILDDIR)/hzlib.o \
  $(BUILDDIR)/memory.o \
  $(BUILDDIR)/font.o \
  $(BUILDDIR)/graphics.o \
  $(BUILDDIR)/window_manager.o \
  $(BUILDDIR)/gui.o \
  $(BUILDDIR)/notepad.o \
  $(BUILDDIR)/about.o \
  $(BUILDDIR)/terminal_app.o

# Toolchain configuration
ifeq ($(TOOLCHAIN),cross)
  CC := i686-elf-gcc
  LD := i686-elf-ld
  CFLAGS_ARCH :=
  LDFLAGS_ARCH :=
else
  CC := gcc
  LD := ld
  CFLAGS_ARCH := -m32
  LDFLAGS_ARCH := -m elf_i386
endif

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -g $(CFLAGS_ARCH) $(INCLUDE_FLAGS)
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

$(BUILDDIR)/filesystem.o: $(DIR_FS)/filesystem.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/ext2.o: $(DIR_FS)/ext2.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Embed ext2.img as a binary blob linked into the kernel.
$(BUILDDIR)/ext2_img.o: ext2.img | $(BUILDDIR)
	objcopy -I binary -O elf32-i386 -B i386 ext2.img $@

# Create a tiny ext2 image on the host (Linux/WSL) if missing.
ext2.img:
	dd if=/dev/zero of=ext2.img bs=1024 count=2048
	mkfs.ext2 -F ext2.img

$(BUILDDIR)/terminal.o: $(DIR_SHELL)/terminal.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/command.o: $(DIR_SHELL)/command.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/printf.o: $(DIR_LIB)/printf.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/hzlib.o: $(DIR_LIB)/hzlib.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/memory.o: $(DIR_LIB)/memory.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/font.o: $(DIR_GUI)/font.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/graphics.o: $(DIR_GUI)/graphics.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/window_manager.o: $(DIR_GUI)/window_manager.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/gui.o: $(DIR_GUI)/gui.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/notepad.o: $(DIR_APPS)/notepad.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/about.o: $(DIR_APPS)/about.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/terminal_app.o: $(DIR_APPS)/terminal_app.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJS) linker.ld | $(BUILDDIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: $(ISO)

$(ISO): $(KERNEL_ELF) grub.cfg | $(BUILDDIR)
	mkdir -p $(ISOROOT)/boot/grub
	cp grub.cfg $(ISOROOT)/boot/grub/grub.cfg
	cp $(KERNEL_ELF) $(ISOROOT)/boot/kernel.elf
	grub-mkrescue -o $@ $(ISOROOT)

run-qemu: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

debug-qemu: $(ISO)
	qemu-system-i386 -cdrom $(ISO) -s -S

clean:
	rm -rf $(BUILDDIR)