# Cubo giratorio en C! puro (sin C, sin SDL2, sin HTML/JS)

Este ejemplo usa **solo C!** para generar 6 frames PPM (`cube_000.ppm` a `cube_005.ppm`) de un cubo giratorio con fondo azul.

## 1) Compilar

```bash
make stage0
./gee examples/rotating_cube_puro.cb /tmp/rotating_cube_puro.s
gcc /tmp/rotating_cube_puro.s -o rotating_cube_puro
```

## 2) Generar los frames

```bash
./rotating_cube_puro
```

## 3) Ver el cubo girar (fuera de terminal)

Convierte los frames a video:

```bash
ffmpeg -framerate 30 -i cube_%03d.ppm -pix_fmt yuv420p cube.mp4
```

Reproduce el video:

```bash
xdg-open cube.mp4
```

En macOS:

```bash
open cube.mp4
```
