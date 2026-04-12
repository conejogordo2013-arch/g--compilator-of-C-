# Cubo giratorio en tiempo real con drawg

## Compilar (modo stub, 100% C!)

```bash
make stage0
./gee libdrawg/drawg.cb /tmp/drawg.s
./gee libdrawg/backend_stub.cb /tmp/drawg_stub.s
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_stub.s -o drawg_cube_app
```

## Ejecutar

```bash
./drawg_cube_app
```

> Nota: el `backend_stub` es para compilar/ejecutar sin backend real (no abre ventana).
> Para abrir ventana real en tiempo real debes enlazar una implementación nativa de `drawg_backend_*`.


## Compilar con backend Linux framebuffer (ventana/salida real sin C)

```bash
./gee libdrawg/backend_linux_fb.cb /tmp/drawg_fb.s
gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_fb.s -o drawg_cube_fb
./drawg_cube_fb
```

> Requiere `/dev/fb0` disponible y permisos de escritura.
