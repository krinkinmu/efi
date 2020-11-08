CC := clang
LD := lld

CFLAGS := \
	-ffreestanding -MMD -mno-red-zone -std=c11 \
	-target x86_64-unknown-windows -Wall -Werror -pedantic
LDFLAGS := -flavor link -subsystem:efi_application -entry:efi_main

KERNEL_CFLAGS := \
	-ffreestanding -MMD -mno-red-zone -std=c11 -Wall -Werror -pedantic
KERNEL_LDFLAGS := \
	-flavor ld -e main

SRCS := main.c clib.c kernel.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bootx64.efi: clib.o main.o
	$(LD) $(LDFLAGS) $^ -out:$@

kernel: kernel.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o kernel.o
	$(LD) $(KERNEL_LDFLAGS) kernel.o -o $@

-include $(SRCS:.c=.d)

.PHONY: clean all default

all: bootx64.efi kernel

clean:
	rm -rf bootx64.efi kernel *.o *.d *.lib
