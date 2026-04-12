# drawg.cb

`drawg` es una librería en **C! puro** para dibujado software 2D en memoria y presentación en tiempo real mediante un backend de ventana.

## API principal

- `drawg_open(canvas, w, h, title)`
- `drawg_clear(canvas, color)`
- `drawg_line(canvas, x0, y0, x1, y1, color)`
- `drawg_present(canvas)`
- `drawg_should_close()`
- `drawg_sleep_ms(ms)`
- `drawg_close(canvas)`

## Contrato de backend (extern)

Tu runtime nativo debe exportar:

- `drawg_backend_init(int32 width, int32 height, string title) -> int32`
- `drawg_backend_should_close() -> int32`
- `drawg_backend_present(int64 pixels, int32 width, int32 height) -> void`
- `drawg_backend_sleep(int32 ms) -> void`
- `drawg_backend_shutdown() -> void`

Con esas funciones, cualquier app C! que use `drawg` abre su ventana y se actualiza en tiempo real.

## Ejemplo

Ver `examples/drawg_rotating_cube.cb`.

## Backend stub

También se incluye `libdrawg/backend_stub.cb` para que compile sin backend real (modo sin ventana).

## Backend Linux framebuffer (puro C!)

También se incluye `libdrawg/backend_linux_fb.cb`, que escribe directamente en `/dev/fb0` sin C/SDL/HTML.

Compilación ejemplo con framebuffer:

```bash
./gee libdrawg/backend_linux_fb.cb /tmp/drawg_fb.s
gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_fb.s -o drawg_cube_fb
```


## Backend Windows GDI (puro C!)

También se incluye `libdrawg/backend_windows_gdi.cb` para compilar `.exe` en Windows sin escribir C.

```bash
./gee libdrawg/backend_windows_gdi.cb /tmp/drawg_win.s
x86_64-w64-mingw32-gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_win.s -lgdi32 -luser32 -o drawg_cube.exe
```


## Guía ARM64 (Termux + Linux)

Ver `examples/drawg_arm64_termux_linux.md` para setup completo y ejecución por entorno.


## Android / APK

Ver `ANDROID_APK_GUIDE.md` para estrategia de empaquetado APK en tiempo real con C! + backend nativo.
