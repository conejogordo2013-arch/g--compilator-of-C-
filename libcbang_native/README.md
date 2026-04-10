# libcbang_native

Implementación en **C! portable** para C!.

## Estado
- Código fuente en `libcbang_native/cbang/*.cb`.
- Build compartido en x86-64/arm64 vía `gee + as + ld`.

## Build (x86-64)

```bash
cd libcbang_native
make all
```

Salida esperada:
- `build/lib/libcbang.so`

## Build directo por script

```bash
bash scripts/build-libcbang-native-shared.sh x86-64 libcbang.so
bash scripts/build-libcbang-native-shared.sh arm-64 libcbang_arm64.so
```

## Test

```bash
bash libcbang_native/tests/test_build_and_run.sh
```

Valida que se genere `build/lib/libcbang.so` en el entorno actual.
