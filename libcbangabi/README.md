# libcbangabi

`libcbangabi` define el ABI oficial y estable para C! (GEE).

## Convenciones ABI

### x86-64
- Calling convention: System V AMD64.
- Args enteros/punteros: registros (`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`), resto por stack.
- Retorno: `rax` (`int32` usa parte baja, `int64/pointer` completo).
- Stack alignment: 16 bytes en calls.

### ARM64
- Calling convention: AAPCS64.
- Args enteros/punteros: `x0..x7`, resto por stack.
- Retorno: `x0`.
- Stack alignment: 16 bytes.

## Tipos ABI
- `int32`: 32 bits con signo.
- `int64`: 64 bits con signo.
- `bool`: `int32` (0 = false, !=0 = true).
- `byte`: 8 bits.
- puntero: `int64` en targets de 64 bits.
- `string`: puntero a bytes terminados en `\0`.

## Layout estable de struct base

```c
struct CbAbiHeader {
  int32 kind;
  int32 flags;
  int64 payload;
};
```

## Símbolos públicos
Todos los símbolos ABI usan prefijo `cbabi_` para evitar colisiones y mantener estabilidad.

## Build `.so`

```bash
bash scripts/build-libcbangabi-shared.sh x86-64 ./libcbangabi.so
bash scripts/build-libcbangabi-shared.sh arm-64 ./libcbangabi_arm64.so
```

## Pruebas

```bash
bash tests/libcbangabi/test_abi_x86_64.sh
bash tests/libcbangabi/test_abi_arm64.sh
```
