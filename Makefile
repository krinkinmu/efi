CC := clang
LD := lld

CFLAGS := -ffreestanding -MMD -mno-red-zone -std=c11 \
	-target x86_64-unknown-windows -Wall -Werror -pedantic
LDFLAGS := -flavor link -subsystem:efi_application -entry:efi_main

SRCS := main.c clib.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

bootx64.efi: clib.o main.o
	$(LD) $(LDFLAGS) $^ -out:$@

-include $(SRCS:.c=.d)

.PHONY: clean all default

all: bootx64.efi

clean:
	rm -rf bootx64.efi *.o *.d *.lib
