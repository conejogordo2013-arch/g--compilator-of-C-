# libcbang

Librería estándar mínima para C! (GEE).

## Módulos
- `base.cb`: impresión y utilidades base.
- `memory.cb`: wrappers simples de memoria.
- `object.cb`: objeto base con ciclo de vida.
- `oops.cb`: dispatch dinámico simple (`cb_oops_call`).
- `collections.cb`: colección dinámica básica.

## Build por arquitectura

### x86-64 (Linux)
```bash
./gee libcbang/base.cb /tmp/base_x86.s
```

### ARM64 (Linux)
```bash
GEE_TARGET=arm-64 ./gee libcbang/base.cb /tmp/base_arm64.s
```

## Uso con app C!
```bash
GEE_BIN=./gee bash scripts/gee-asm-link.sh x86-64 app.cb app_bin /tmp/libcbang_module.s
```

## `.so` / `.dll`
`libcbang` está implementada como código fuente `.cb` portable por target. Para empaquetar como `.so` o `.dll`, compila a ASM/OBJ con `gee` + assembler de tu plataforma y enlaza con tu linker del sistema (Linux: `ld`/`gcc -shared`; Windows: `lld-link`/`link.exe`).
