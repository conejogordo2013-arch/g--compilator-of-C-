# C! Quickstart (compilar y ejecutar)

## 1) Construir el compilador base

```bash
make stage0
```

Genera `./gee`.

## 2) Compilar + ejecutar en un comando (modo no-cc recomendado)

```bash
bash scripts/gee-run.sh examples/no_cc.cb --target x86-64 --mode no-cc --out demo_x86
```

Ese flujo usa:

- `gee` para `C! -> ASM`
- `as` para `ASM -> OBJ`
- `ld` para `OBJ -> BIN`

Sin pasar por `cc/gcc/clang`.

## 3) Comandos básicos manuales

### x86-64 sin C compiler drivers

```bash
GEE_BIN=./gee bash scripts/gee-asm-link.sh x86-64 examples/no_cc.cb no_cc_demo
./no_cc_demo
echo $?
```

### ARM64 sin C compiler drivers

```bash
GEE_BIN=./gee bash scripts/gee-asm-link.sh arm-64 examples/no_cc.cb no_cc_arm64
```

Si tu host no es ARM64, ejecútalo con emulación:

```bash
qemu-aarch64 ./no_cc_arm64
```

## 4) Build stage1 selfhost sin CC (jalada fuerte)

```bash
make bootstrap-no-cc
make bootstrap-no-cc-smoke
```

Esto construye `gee_stage1_nocc` con `gee + as + ld`.


## Setup completo

Para guía total x86-64 + ARM64 (compilador, librerías, ejemplos, troubleshooting), ver `COMPLETE_SETUP_X86_64_ARM64.md`.
