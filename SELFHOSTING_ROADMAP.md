# GEE Self-hosting Roadmap (C! only)

Este documento define el plan técnico para llegar a un compilador GEE 100% en C!.

## Estado actual

- Frontend/backend de producción viven en `*.c` (`lexer.c`, `parser.c`, `ast.c`, `codegen.c`, `main.c`).
- Existe un stage1 en `selfhost/*.cb`, pero todavía es de validación de pipeline.
- `bootstrap-no-cc` ya evita `cc/gcc/clang` para compilar programas C! y para generar `gee_stage1_nocc`.

## Criterios de "done" para self-host real

1. Implementación completa del compilador en C! (lexer+parser+AST+IR+x86-64+arm64).
2. `make selfhost-release` construye GEE sin depender de fuentes C.
3. `gee_selfhost` compila su propio código y pasa pruebas de compatibilidad.
4. El pipeline de binarios usa solo `gee_selfhost + as + ld`.

## Plan por fases

### Fase 1 — Paridad de lenguaje necesaria para portar el compilador

- [x] Aritmética de punteros básica en expresiones aditivas (`ptr +/- int`, `int + ptr`).
- [ ] Soporte estable para buffers dinámicos en C! (estructuras + utilidades de memoria).
- [ ] Pruebas de regresión para patrones de lexer/parser self-host.

### Fase 2 — Frontend self-host

- [ ] `selfhost/token.cb`: kinds, keywords, token buffer.
- [ ] `selfhost/lexer.cb`: tokenización completa (comentarios, strings, números, operadores).
- [ ] `selfhost/parser.cb`: AST completo y chequeo de tipos mínimo.

### Fase 3 — Backend self-host

- [ ] `selfhost/ir.cb`: emisión de IR.
- [ ] `selfhost/codegen_x86_64.cb` y `selfhost/codegen_arm64.cb`.
- [ ] Emisión de ASM para ambos targets.

### Fase 4 — Bootstrap y compatibilidad

- [ ] `make selfhost-release` (sin compilar `*.c`).
- [ ] Autocompilación: `gee_selfhost` compila `selfhost/*.cb`.
- [ ] Matriz de compatibilidad con ejemplos y benchmarks existentes.

## Comandos de verificación objetivo

```bash
make selfhost-audit
make bootstrap-no-cc
make bootstrap-no-cc-smoke
```
