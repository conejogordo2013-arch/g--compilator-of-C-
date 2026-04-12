# Cubo verde giratorio en fondo azul (Windows .exe, C! puro)

Este ejemplo usa:
- `libdrawg/drawg.cb`
- `examples/drawg_rotating_cube.cb`
- `libdrawg/backend_windows_gdi.cb`

## Compilar a .exe

```bash
make stage0
./gee libdrawg/drawg.cb /tmp/drawg.s
./gee libdrawg/backend_windows_gdi.cb /tmp/drawg_win.s
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
x86_64-w64-mingw32-gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_win.s -lgdi32 -luser32 -o drawg_cube.exe
```

## Ejecutar

Abre `drawg_cube.exe` y verás el canvas en tiempo real con fondo azul y cubo girando.
