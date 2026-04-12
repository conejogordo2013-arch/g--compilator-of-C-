# C! Setup total (x86-64 + ARM64)

Guía completa para dejar **todo funcionando**: compilador, librerías, ejemplos y flujo de comandos.

---

## 0) Requisitos

### Linux x86-64 / ARM64 nativo

```bash
sudo apt-get update
sudo apt-get install -y build-essential binutils make bash
```

Opcional (tests extra y conversión video):

```bash
sudo apt-get install -y qemu-user ffmpeg
```

### Cross ARM64 desde host x86-64 (opcional)

```bash
sudo apt-get install -y gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu qemu-user
```

---

## 1) Clonar y entrar

```bash
git clone <repo>
cd GEE-compilator-of-C-
```

---

## 2) Compilar el compilador C! (stage0)

```bash
make stage0
```

Salida esperada: binario `./gee`.

Verificar:

```bash
./gee --help || true
```

---

## 3) Comandos base de compilación C!

### A) C! -> ASM

```bash
./gee examples/hola_mundo.cb /tmp/hola.s
```

### B) ASM -> binario (x86-64)

```bash
bash scripts/gee-asm-link.sh x86-64 examples/hola_mundo.cb hola_x86
./hola_x86
```

### C) ASM -> binario (arm-64)

```bash
bash scripts/gee-asm-link.sh arm-64 examples/hola_mundo.cb hola_arm64
```

Si estás en host x86-64:

```bash
qemu-aarch64 ./hola_arm64
```

---

## 4) Compilar librerías `drawg`

```bash
./gee libdrawg/drawg.cb /tmp/drawg.s
./gee libdrawg/backend_stub.cb /tmp/drawg_stub.s
./gee libdrawg/backend_linux_fb.cb /tmp/drawg_fb.s
./gee libdrawg/backend_windows_gdi.cb /tmp/drawg_win.s
```

---

## 5) Cubo en tiempo real (x86-64 Linux, modo stub)

```bash
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_stub.s -o drawg_cube_stub
./drawg_cube_stub
```

> Stub compila/ejecuta sin ventana real.

---

## 6) Cubo en tiempo real Linux ARM64 con framebuffer

```bash
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_fb.s -o drawg_cube_fb
./drawg_cube_fb
```

Requisitos:
- `/dev/fb0` existente
- permisos (`video` o root)
- sesión local (TTY real)

---

## 7) Cubo para Windows `.exe` (cross desde Linux)

```bash
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
x86_64-w64-mingw32-gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_win.s -lgdi32 -luser32 -o drawg_cube.exe
```

Ejecutar en Windows: doble click a `drawg_cube.exe`.

---

## 8) Ruta universal sin backend (PPM + video)

```bash
./gee examples/rotating_cube_puro.cb /tmp/rotating_cube_puro.s
gcc /tmp/rotating_cube_puro.s -o rotating_cube_puro
./rotating_cube_puro
ffmpeg -framerate 30 -i cube_%03d.ppm -pix_fmt yuv420p cube.mp4
```

---

## 9) Tests recomendados

```bash
bash scripts/test-ogl.sh
bash scripts/test-ogl2d.sh
bash scripts/test-ogl3d.sh
bash scripts/test-language-runtime.sh
```

---

## 10) Problemas típicos y solución rápida

1. **`./gee: No such file or directory`**
   - Ejecuta `make stage0`.

2. **`x86_64-w64-mingw32-gcc: command not found`**
   - Instala MinGW: `sudo apt-get install -y gcc-mingw-w64-x86-64`.

3. **No dibuja con framebuffer**
   - Verifica `/dev/fb0`, permisos y TTY local.

4. **ARM64 binario no corre en x86-64**
   - Usa `qemu-aarch64`.

---

## 11) Flujo mínimo recomendado “todo bien”

```bash
make stage0
./gee libdrawg/drawg.cb /tmp/drawg.s
./gee libdrawg/backend_stub.cb /tmp/drawg_stub.s
./gee examples/drawg_rotating_cube.cb /tmp/drawg_cube.s
gcc /tmp/drawg_cube.s /tmp/drawg.s /tmp/drawg_stub.s -o drawg_cube_stub
./drawg_cube_stub
```

Si ese flujo pasa, tu toolchain C! está sano.


---

## 12) Compilar con GCC, Clang y modo no-cc (compilador propio)

### A) Con GCC (flujo clásico)

```bash
./gee examples/hola_mundo.cb /tmp/hola.s
gcc /tmp/hola.s -o hola_gcc
./hola_gcc
```

### B) Con Clang

```bash
./gee examples/hola_mundo.cb /tmp/hola.s
clang /tmp/hola.s -o hola_clang
./hola_clang
```

### C) Con toolchain propio C! (no-cc mode)

Esto evita `gcc/clang` y usa `as + ld`:

```bash
bash scripts/gee-run.sh examples/no_cc.cb --target x86-64 --mode no-cc --out no_cc_x86
./no_cc_x86
```

Manual equivalente:

```bash
GEE_BIN=./gee bash scripts/gee-asm-link.sh x86-64 examples/no_cc.cb no_cc_manual
./no_cc_manual
```

### D) no-cc en ARM64

```bash
GEE_BIN=./gee bash scripts/gee-asm-link.sh arm-64 examples/no_cc.cb no_cc_arm64
qemu-aarch64 ./no_cc_arm64   # si host no es ARM64
```

### E) Cuándo usar cada uno

- **GCC/Clang**: mejor cuando enlazas libc, backends nativos, o cross-toolchains (ej. Windows `.exe`).
- **no-cc**: mejor para validar pipeline propio C! puro (ASM+AS+LD).
- **drawg/backends**: normalmente necesitarás GCC/Clang porque hay símbolos externos de plataforma.

### F) Perfilado y diagnósticos

Scripts útiles:

```bash
bash scripts/gee-profile.sh examples/no_cc.cb --target x86-64 --mode no-cc --out prof_demo
bash scripts/gee-doctor.sh
```
