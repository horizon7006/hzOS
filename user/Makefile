CC := gcc
CFLAGS := -m32 -ffreestanding -O2 -Wall -Wextra -fno-pic -fno-pie -nostdlib
LDFLAGS := -m elf_i386 -T user.ld

COMMON_OBJS := crt0.o libhz.o

all: hello.prog about.prog notepad.prog terminal.prog

%.prog: %.o $(COMMON_OBJS)
	ld $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) -m32 -c $< -o $@

clean:
	rm -f *.o *.prog
