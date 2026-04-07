GEE ?= gee
AS ?= as
LD ?= ld
NASM ?= nasm

GEE_LOCAL := ../gee
ifneq ($(wildcard $(GEE_LOCAL)),)
GEE_CMD := $(GEE_LOCAL)
GEE_DEP := $(GEE_LOCAL)
else
GEE_CMD := $(GEE)
GEE_DEP :=
endif

all: bootloader.bin kernel.s kernel.o kernel.elf

bootloader.bin: bootloader.asm
	$(NASM) -f bin $< -o $@

kernel.s: kernel.cb $(GEE_DEP)
	$(GEE_CMD) kernel.cb kernel.s

kernel.o: kernel.s
	$(AS) --64 kernel.s -o kernel.o

kernel.elf: kernel.o linker.ld
	$(LD) -nostdlib -z max-page-size=0x1000 -T linker.ld kernel.o -o kernel.elf

clean:
	rm -f bootloader.bin kernel.s kernel.o kernel.elf

.PHONY: all clean
