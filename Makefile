
# TOOLCHAIN selection:
#   Native 32-bit: make TOOLCHAIN=native    (default)
#   Cross i686-elf: make TOOLCHAIN=cross

TOOLCHAIN ?= native

SRCDIR   := src
BUILDDIR := build
ISOROOT  := iso

KERNEL_ELF := $(BUILDDIR)/kernel.elf
ISO        := $(BUILDDIR)/hzOS.iso

OBJS := \
  $(BUILDDIR)/kernel_entry.o \
  $(BUILDDIR)/kernel.o \
  $(BUILDDIR)/terminal.o \
  $(BUILDDIR)/printf.o \
  $(BUILDDIR)/command.o \
  $(BUILDDIR)/filesystem.o \
  $(BUILDDIR)/gdt.o \
  $(BUILDDIR)/gdt_flush.o \
  $(BUILDDIR)/idt.o \
  $(BUILDDIR)/isr.o \
  $(BUILDDIR)/isr_stub.o \
  $(BUILDDIR)/keyboard.o

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

CFLAGS := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -g $(CFLAGS_ARCH)
LDFLAGS := -nostdlib -T linker.ld $(LDFLAGS_ARCH)

.PHONY: all kernel iso run-qemu debug-qemu clean

all: $(ISO)

kernel: $(KERNEL_ELF)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/kernel.o: $(SRCDIR)/kernel.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/kernel_entry.o: $(SRCDIR)/kernel_entry.s | $(BUILDDIR)
	$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(KERNEL_ELF): $(OBJS) linker.ld | $(BUILDDIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(BUILDDIR)/terminal.o: $(SRCDIR)/terminal.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/printf.o: $(SRCDIR)/printf.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/command.o: $(SRCDIR)/command.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/filesystem.o: $(SRCDIR)/filesystem.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/gdt.o: $(SRCDIR)/gdt.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/gdt_flush.o: $(SRCDIR)/gdt_flush.s | $(BUILDDIR)
	$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/idt.o: $(SRCDIR)/idt.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/isr.o: $(SRCDIR)/isr.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/isr_stub.o: $(SRCDIR)/isr_stub.s | $(BUILDDIR)
	$(CC) $(CFLAGS_ARCH) -c $< -o $@

$(BUILDDIR)/keyboard.o: $(SRCDIR)/keyboard.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

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