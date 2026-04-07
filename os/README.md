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

## Termux (usar `gee` en cualquier carpeta)

Instala el compilador en tu PATH de Termux:

```bash
make install
```

O usa menú de arquitectura frontend (x86 / x86-64 / ARM-v7 / ARM-64):

```bash
make install-menu
```

Esto copia `gee` a `/data/data/com.termux/files/usr/bin/gee` (por defecto).
Si quieres otro destino:

```bash
make install PREFIX=$PREFIX
```

Después puedes compilar desde cualquier directorio con:

```bash
gee archivo.cb archivo.s
```

También quedan disponibles wrappers directos:

```bash
gee-x86 archivo.cb archivo.s
gee-x86-64 archivo.cb archivo.s
gee-arm-v7 archivo.cb archivo.s
gee-arm-64 archivo.cb archivo.s
```

Cambiar/consultar target activo:

```bash
gee-target list
gee-target show
gee-target set arm-64
```

Diagnóstico rápido de instalación:

```bash
gee-doctor
```

Si alguna vez quieres quitarlo:

```bash
make uninstall
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

### Nota sobre el error `No rule to make target '../gee'`

`os/Makefile` ahora intenta usar `../gee` si existe (workflow de repo), y si no, usa `gee` del PATH. Así funciona tanto dentro del repo como globalmente en Termux.
