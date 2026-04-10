# libcbang_native

Implementación base en C modular para C!.

## Módulos
- `core`: tipos/macros/utilidades básicas.
- `memory`: wrappers `malloc/free` + arena allocator.
- `string`: string dinámico (set/append/slice/compare).
- `io`: print/read/write de archivos.
- `containers`: vector dinámico, lista doble, hashmap básico.
- `runtime`: init/shutdown/panic runtime.
- `abi`: ABI público estable para interoperabilidad.

## Build

```bash
cd libcbang_native
make all
```

Genera:
- librerías por módulo (`build/lib/libcbang_*.a`)
- agregadas (`build/lib/libcbang.a`, `build/lib/libcbang.so`)
- ejemplos (`build/examples/minimal_example`, `build/examples/full_example`)

## Build minimal

```bash
make minimal
```

Solo compila `core + memory`.

## Optimización y DCE

El `Makefile` usa:
- `-O2`
- `-ffunction-sections`
- `-fdata-sections`
- `-Wl,--gc-sections`

para reducir tamaño y permitir eliminar código no usado.
