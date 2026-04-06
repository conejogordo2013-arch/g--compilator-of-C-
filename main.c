#include "ast.h"
#include "codegen.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)n, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

typedef enum BackendFlavor {
    BACKEND_GAS = 0,
    BACKEND_NASM
} BackendFlavor;

static int has_cb_extension(const char *path) {
    size_t n = strlen(path);
    return n >= 3 && strcmp(path + n - 3, ".cb") == 0;
}

static char *default_output_path(const char *input, BackendFlavor backend) {
    size_t n = strlen(input);
    char *out = (char *)malloc(n + 3);
    if (!out) return NULL;
    memcpy(out, input, n + 1);
    char *dot = strrchr(out, '.');
    if (dot) strcpy(dot, backend == BACKEND_NASM ? ".asm" : ".s");
    else strcat(out, backend == BACKEND_NASM ? ".asm" : ".s");
    return out;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: gee [-f gas|nasm] [--emit-ir ir.txt] <input.cb> [output.s|output.asm]\n");
        return 1;
    }

    BackendFlavor backend = BACKEND_GAS;
    int argi = 1;
    const char *ir_dump_path = NULL;
    while (argi < argc && argv[argi][0] == '-') {
        if (strcmp(argv[argi], "-f") == 0 && argi + 1 < argc) {
            if (strcmp(argv[argi + 1], "gas") == 0) backend = BACKEND_GAS;
            else if (strcmp(argv[argi + 1], "nasm") == 0) backend = BACKEND_NASM;
            else {
                fprintf(stderr, "error: unsupported backend '%s' (use gas or nasm)\n", argv[argi + 1]);
                return 1;
            }
            argi += 2;
            continue;
        }
        if (strcmp(argv[argi], "--emit-ir") == 0 && argi + 1 < argc) {
            ir_dump_path = argv[argi + 1];
            argi += 2;
            continue;
        }
        break;
    }

    if (argi >= argc) {
        fprintf(stderr, "error: missing input file\n");
        return 1;
    }
    const char *in_path = argv[argi];
    if (!has_cb_extension(in_path)) {
        fprintf(stderr, "error: input file must use .cb extension\n");
        return 1;
    }
    char *auto_out = default_output_path(in_path, backend);
    const char *out_path = (argc > argi + 1) ? argv[argi + 1] : (auto_out ? auto_out : "out.s");

    char *src = read_file(in_path);
    if (!src) {
        fprintf(stderr, "Failed to read input '%s'\n", in_path);
        free(auto_out);
        return 1;
    }

    Parser p = {0};
    AstNode *program = parse_program(&p, src);

    if (p.had_error) {
        fprintf(stderr, "Compilation failed due to parse/type errors.\n");
        /* NOTE: bootstrap mode intentionally avoids teardown because front-end
         * ownership is still being stabilized during self-hosting migration. */
        free(src);
        free(auto_out);
        return 1;
    }

    IRProgram ir;
    generate_ir(program, &ir);
    if (ir_dump_path) {
        FILE *ir_out = fopen(ir_dump_path, "w");
        if (!ir_out) {
            fprintf(stderr, "Failed to write IR dump '%s'\n", ir_dump_path);
            ir_free(&ir);
            free(src);
            free(auto_out);
            return 1;
        }
        emit_ir_text(&ir, ir_out);
        fclose(ir_out);
    }

    FILE *out = fopen(out_path, "w");
    if (!out) {
        fprintf(stderr, "Failed to write assembly output '%s'\n", out_path);
        ir_free(&ir);
        free(src);
        free(auto_out);
        return 1;
    }
    if (backend == BACKEND_NASM) emit_nasm_x86_64(&ir, out);
    else emit_x86_64(&ir, out);
    fclose(out);

    printf("GEE: compiled %s -> %s (%s)\n", in_path, out_path, backend == BACKEND_NASM ? "nasm" : "gas");

    ir_free(&ir);
    (void)program;
    (void)p;
    free(src);
    free(auto_out);
    return 0;
}
