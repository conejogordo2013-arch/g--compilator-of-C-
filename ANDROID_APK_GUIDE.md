# C! + APK (Android) en tiempo real: guía práctica

Sí: **C! puede vivir perfectamente en tiempo real**.
No es un límite del lenguaje, es tema de **toolchain + backend de plataforma**.

## Idea clave

Tu arquitectura para Android debe ser:

1. **Core gráfico en C!** (por ejemplo `libdrawg/drawg.cb` + lógica 3D)
2. **Backend Android** que presente píxeles en pantalla (Surface/ANativeWindow/OpenGL ES)
3. **APK** (Kotlin/Java + Gradle) que cargue la librería nativa y abra la UI

---

## Ruta recomendada (realista)

### A) Mantén el motor en C!

- Raster/lineas/cámara/cubo en C!.
- API estable tipo `drawg_*`.

### B) Crea backend Android dedicado

Nuevo archivo sugerido: `libdrawg/backend_android_ndk.cb`.
Debe implementar el contrato:

- `drawg_backend_init(...)`
- `drawg_backend_should_close()`
- `drawg_backend_present(...)`
- `drawg_backend_sleep(...)`
- `drawg_backend_shutdown()`

En Android, `drawg_backend_present` puede:
- copiar el buffer a `ANativeWindow` (RGBA8888), o
- subir textura a OpenGL ES/Vulkan.

### C) Compilar C! a objeto ARM64

Flujo típico:

```bash
./gee libdrawg/drawg.cb /tmp/drawg.s
./gee libdrawg/backend_android_ndk.cb /tmp/drawg_android.s
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
```

Luego usar Android NDK (`aarch64-linux-androidXX-clang`) para generar `.o/.so`.

### D) Empaquetar en APK

- Android Studio / Gradle
- `externalNativeBuild` o tarea custom
- `System.loadLibrary("drawg_android")`
- Activity con `SurfaceView`/`TextureView`

Resultado: abres el APK y ves el cubo en tiempo real.

---

## ¿Sin root? ¿Con root?

- **Sin root**: APK normal (recomendado)
- **Con root**: acceso extra (framebuffer directo, etc.), pero no es necesario para una app bien hecha

---

## ¿Qué falta hoy para cerrar el ciclo?

1. Backend Android real (`backend_android_ndk.cb`)
2. Plantilla mínima de proyecto Android (Gradle + Kotlin + carga de `.so`)
3. Script de build reproducible (`scripts/build-android-apk.sh`)

Con eso, C! queda con pipeline completo a APK real-time.

---

## Mensaje importante

Tu lectura es correcta: **C! no es “malo” ni “incompleto” por usar PPM en algunos entornos**.
PPM es solo fallback universal. El modo serio de tiempo real es backend nativo por plataforma (`.apk`, `.exe`, Linux native).
