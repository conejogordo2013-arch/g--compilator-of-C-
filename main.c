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

static int write_output(const char *path, IRProgram *ir) {
    FILE *out = fopen(path, "w");
    if (!out) return 1;
    emit_x86_64(ir, out);
    fclose(out);
    return 0;
}

static int has_cb_extension(const char *path) {
    size_t n = strlen(path);
    return n >= 3 && strcmp(path + n - 3, ".cb") == 0;
}

static char *default_output_path(const char *input) {
    size_t n = strlen(input);
    char *out = (char *)malloc(n + 3);
    if (!out) return NULL;
    memcpy(out, input, n + 1);
    char *dot = strrchr(out, '.');
    if (dot) strcpy(dot, ".s");
    else strcat(out, ".s");
    return out;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: gee <input.cb> [output.s]\n");
        return 1;
    }

    const char *in_path = argv[1];
    if (!has_cb_extension(in_path)) {
        fprintf(stderr, "error: input file must use .cb extension\n");
        return 1;
    }
    char *auto_out = default_output_path(in_path);
    const char *out_path = (argc > 2) ? argv[2] : (auto_out ? auto_out : "out.s");

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

    if (write_output(out_path, &ir) != 0) {
        fprintf(stderr, "Failed to write assembly output '%s'\n", out_path);
        ir_free(&ir);
        free(src);
        free(auto_out);
        return 1;
    }

    printf("GEE: compiled %s -> %s\n", in_path, out_path);

    ir_free(&ir);
    (void)program;
    (void)p;
    free(src);
    free(auto_out);
    return 0;
}
