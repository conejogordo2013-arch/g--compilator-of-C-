# C! OS Prototype

Este directorio contiene una base mínima para un OS en C!:

- `bootloader.asm`: bootloader BIOS de 512 bytes (ASM, real mode), con firma `0xAA55`.
- `kernel.cb`: kernel mínimo en C! con entrada `kmain`.
- `linker.ld`: script de link para el kernel ELF.
- `Makefile`: construye bootloader y kernel.

## Build

Desde la raíz del repo:

```bash
make stage0
make -C os
```

Esto genera:

- `os/bootloader.bin`
- `os/kernel.s`
- `os/kernel.o`
- `os/kernel.elf`

## Estado actual

- El bootloader imprime mensajes y se detiene.
- El kernel en C! ya se compila con `gee` y se enlaza como ELF freestanding.
- Siguiente paso para hacerlo booteable end-to-end: cargar `kernel.elf` desde disco (INT 13h o segundo stage) y saltar a su entrypoint.
