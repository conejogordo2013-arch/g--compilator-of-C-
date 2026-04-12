# libogl (C!)

`libogl` es una librería de gráficos en **C! puro** para empezar rápido al instalar el lenguaje.

## Objetivo
- API simple tipo canvas 2D.
- Funciona en software sin depender obligatoriamente de OpenGL.
- Backends opcionales (OpenGL/SDL/etc.) mediante `backend.cb`.

## Módulos
- `color.cb`: empaquetado RGBA y colores base.
- `canvas.cb`: canvas, píxeles, rectángulos, líneas y checksum.
- `image.cb`: blit de imágenes a canvas.
- `backend.cb`: interfaz opcional para presentar en backends externos.

## Filosofía
OpenGL es opcional: `libogl` no queda amarrada solo a OpenGL.
