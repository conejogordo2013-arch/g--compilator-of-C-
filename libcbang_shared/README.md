# libcbang_shared

Pipeline para generar `libcbang_shared.so` desde módulos C!.

## Build

```bash
bash scripts/build-libcbang-shared.sh x86-64 ./libcbang_shared.so
bash scripts/build-libcbang-shared.sh arm-64 ./libcbang_shared_arm64.so
```

## Test de enlace y ejecución

```bash
bash tests/libcbang_shared/test_shared_link.sh
bash tests/libcbang_shared/test_shared_arm64.sh
```

- `test_shared_link.sh` valida build/link/run en x86-64 con `EXIT:0`.
- `test_shared_arm64.sh` valida build/link ARM64 y ejecuta con `qemu-aarch64` si está disponible.

El pipeline usa:
1. `gee module.cb module.s`
2. `as module.s -o module.o`
3. `ld -shared *.o -o libcbang_shared.so`
