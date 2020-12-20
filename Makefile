CC := clang
LD := lld
ARCH ?= x86-64

ifeq ($(ARCH),x86-64)
include x86-64.env
else
include aarch64.env
endif

export

SRCS := main.c clib.c io.c loader.c config.c log.c kernel.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

boot.efi: clib.o io.o loader.o config.o log.o main.o
	$(LD) $(LDFLAGS) $^ -out:$@

kernel.elf: kernel.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o kernel.o
	$(LD) $(KERNEL_LDFLAGS) kernel.o -o $@

-include $(SRCS:.c=.d)

.PHONY: clean all default

all: boot.efi kernel.elf

clean:
	rm -rf *.efi *.elf *.o *.d *.lib
