# Guía ARM64 (Termux + Linux ARM64) para ver el cubo girando con C!

Esta guía explica **cómo configurar C!**, compilar y ejecutar según tu entorno ARM64.

---

## 1) Preparar C! (común)

```bash
git clone <tu-repo-cbang>
cd GEE-compilator-of-C-
make stage0
```

Eso genera `./gee`.

---

## 2) Linux ARM64 nativo (Raspberry Pi, SBC, servidor ARM con framebuffer)

Si estás en Linux ARM64 real y tienes `/dev/fb0`, usa backend framebuffer puro C!:

```bash
./gee libdrawg/drawg.cb /tmp/drawg.s
./gee libdrawg/backend_linux_fb.cb /tmp/drawg_fb.s
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_fb.s -o drawg_cube_fb
```

Ejecutar:

```bash
./drawg_cube_fb
```

### Si no dibuja nada

1. Verifica framebuffer:
```bash
ls -l /dev/fb0
```
2. Ejecuta en TTY real (no SSH) si hace falta.
3. Asegura permisos sobre `/dev/fb0` (grupo `video` o root).

---

## 3) Termux ARM64 (Android)

En Termux normalmente **no existe `/dev/fb0` usable** para dibujar directo, así que la ruta estable es el modo pure C! por frames PPM:

```bash
./gee examples/rotating_cube_puro.cb /tmp/rotating_cube_puro.s
gcc /tmp/rotating_cube_puro.s -o rotating_cube_puro
./rotating_cube_puro
```

Genera `cube_000.ppm ... cube_005.ppm`.

Convertir a video y abrir en Android:

```bash
ffmpeg -framerate 30 -i cube_%03d.ppm -pix_fmt yuv420p cube.mp4
termux-open cube.mp4
```

---

## 4) ¿Y el backend Windows GDI?

Ese backend (`backend_windows_gdi.cb`) es para compilar `.exe` en Windows (o cross-compilar con MinGW), no para Termux/Linux framebuffer.

---

## 5) Recomendación práctica por plataforma

- **Linux ARM64 con `/dev/fb0`** → `backend_linux_fb.cb` (tiempo real).
- **Termux ARM64** → `rotating_cube_puro.cb` + `ffmpeg` (video fuera de terminal).
- **Windows** → `backend_windows_gdi.cb` para `.exe`.
