#include "ast.h"
#include "codegen.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void append_buf(char **buf, size_t *len, size_t *cap, const char *text, size_t n) {
    if (*len + n + 1 > *cap) {
        size_t new_cap = (*cap == 0) ? 1024 : *cap;
        while (*len + n + 1 > new_cap) new_cap *= 2;
        char *nb = (char *)realloc(*buf, new_cap);
        if (!nb) return;
        *buf = nb;
        *cap = new_cap;
    }
    if (*buf) {
        memcpy(*buf + *len, text, n);
        *len += n;
        (*buf)[*len] = '\0';
    }
}

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

static char *dirname_of(const char *path) {
    const char *slash = strrchr(path, '/');
    if (!slash) {
        char *d = (char *)malloc(2);
        if (!d) return NULL;
        d[0] = '.';
        d[1] = '\0';
        return d;
    }
    size_t n = (size_t)(slash - path);
    char *d = (char *)malloc(n + 1);
    if (!d) return NULL;
    memcpy(d, path, n);
    d[n] = '\0';
    return d;
}

static char *join_path(const char *a, const char *b) {
    size_t na = strlen(a), nb = strlen(b);
    int need_sep = (na > 0 && a[na - 1] != '/');
    char *p = (char *)malloc(na + nb + (need_sep ? 2 : 1));
    if (!p) return NULL;
    memcpy(p, a, na);
    if (need_sep) p[na++] = '/';
    memcpy(p + na, b, nb);
    p[na + nb] = '\0';
    return p;
}

typedef struct PreCtx {
    char **once_files;
    size_t once_count, once_cap;
    char **macros;
    size_t macro_count, macro_cap;
} PreCtx;

static int list_contains(char **arr, size_t n, const char *v) {
    for (size_t i = 0; i < n; ++i) if (strcmp(arr[i], v) == 0) return 1;
    return 0;
}

static void list_add_unique(char ***arr, size_t *n, size_t *cap, const char *v) {
    if (list_contains(*arr, *n, v)) return;
    if (*n + 1 > *cap) {
        size_t nc = (*cap == 0) ? 8 : (*cap * 2);
        char **na = (char **)realloc(*arr, nc * sizeof(char *));
        if (!na) return;
        *arr = na;
        *cap = nc;
    }
    size_t len = strlen(v);
    char *cpy = (char *)malloc(len + 1);
    if (!cpy) return;
    memcpy(cpy, v, len + 1);
    (*arr)[(*n)++] = cpy;
}

static void prec_free(PreCtx *ctx) {
    for (size_t i = 0; i < ctx->once_count; ++i) free(ctx->once_files[i]);
    for (size_t i = 0; i < ctx->macro_count; ++i) free(ctx->macros[i]);
    free(ctx->once_files);
    free(ctx->macros);
}

static const char *skip_ws(const char *s, const char *end) {
    while (s < end && isspace((unsigned char)*s)) s++;
    return s;
}

static void parse_ident(const char **p, const char *end, char *out, size_t cap) {
    size_t n = 0;
    while (*p < end && (isalnum((unsigned char)**p) || **p == '_')) {
        if (n + 1 < cap) out[n++] = **p;
        (*p)++;
    }
    out[n] = '\0';
}

static int preprocess_file_recursive(const char *path, PreCtx *ctx, char **out, size_t *out_len, size_t *out_cap, int depth);

static int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int try_import_candidate(const char *root, const char *mod, PreCtx *ctx, char **out, size_t *out_len, size_t *out_cap, int depth) {
    char rel[256];
    snprintf(rel, sizeof(rel), "stdlib/%s.h", mod);
    char *cand = join_path(root, rel);
    if (!cand) return 0;
    int ok = file_exists(cand) && preprocess_file_recursive(cand, ctx, out, out_len, out_cap, depth + 1);
    free(cand);
    return ok;
}

static int try_import_roots_list(const char *roots, const char *mod, PreCtx *ctx, char **out, size_t *out_len, size_t *out_cap, int depth) {
    if (!roots || !*roots) return 0;
    size_t n = strlen(roots);
    char *buf = (char *)malloc(n + 1);
    if (!buf) return 0;
    memcpy(buf, roots, n + 1);
    int ok = 0;
    for (char *tok = buf; tok && *tok; ) {
        char *next = strchr(tok, ':');
        if (next) {
            *next = '\0';
            next++;
        }
        if (*tok == '\0') {
            tok = next;
            continue;
        }
        if (try_import_candidate(tok, mod, ctx, out, out_len, out_cap, depth)) {
            ok = 1;
            break;
        }
        tok = next;
    }
    free(buf);
    return ok;
}

static int preprocess_import_module(const char *base_dir, const char *mod, PreCtx *ctx, char **out, size_t *out_len, size_t *out_cap, int depth) {
    if (try_import_candidate(base_dir, mod, ctx, out, out_len, out_cap, depth)) return 1;
    if (try_import_candidate(".", mod, ctx, out, out_len, out_cap, depth)) return 1;
    if (try_import_roots_list(getenv("GEE_STDLIB"), mod, ctx, out, out_len, out_cap, depth)) return 1;
    if (try_import_roots_list(getenv("GEE_PATH"), mod, ctx, out, out_len, out_cap, depth)) return 1;
    fprintf(stderr,
            "error: import module '%s' not found. searched: <file_dir>/stdlib, ./stdlib, GEE_STDLIB, GEE_PATH\n",
            mod);
    return 0;
}

static int preprocess_file_recursive(const char *path, PreCtx *ctx, char **out, size_t *out_len, size_t *out_cap, int depth) {
    if (depth > 32) {
        fprintf(stderr, "error: include depth exceeded at '%s'\n", path);
        return 0;
    }
    if (list_contains(ctx->once_files, ctx->once_count, path)) return 1;
    char *src = read_file(path);
    if (!src) {
        fprintf(stderr, "error: include not found: %s\n", path);
        return 0;
    }

    char *base = dirname_of(path);
    if (!base) { free(src); return 0; }

    int active_stack[64];
    int cond_stack[64];
    int sp = 0;
    active_stack[sp++] = 1;
    cond_stack[0] = 1;

    const char *p = src;
    while (*p) {
        const char *line_start = p;
        while (*p && *p != '\n') p++;
        const char *line_end = p;
        if (*p == '\n') p++;

        const char *q = skip_ws(line_start, line_end);
        int active = active_stack[sp - 1];
        if (active && strncmp(q, "import", 6) == 0 && (q + 6 < line_end) && isspace((unsigned char)q[6])) {
            q += 6;
            q = skip_ws(q, line_end);
            char mod[128];
            parse_ident(&q, line_end, mod, sizeof(mod));
            q = skip_ws(q, line_end);
            if (mod[0] && q < line_end && *q == ';') {
                if (!preprocess_import_module(base, mod, ctx, out, out_len, out_cap, depth)) {
                    free(base);
                    free(src);
                    return 0;
                }
                continue;
            }
        }
        if (q < line_end && *q == '#') {
            q++;
            q = skip_ws(q, line_end);
            if (strncmp(q, "include", 7) == 0) {
                q += 7; q = skip_ws(q, line_end);
                if (active && q < line_end && *q == '"') {
                    q++;
                    const char *r = q;
                    while (r < line_end && *r != '"') r++;
                    if (r < line_end) {
                        char inc[512];
                        size_t n = (size_t)(r - q);
                        if (n >= sizeof(inc)) n = sizeof(inc) - 1;
                        memcpy(inc, q, n);
                        inc[n] = '\0';
                        char *full = join_path(base, inc);
                        if (!full || !preprocess_file_recursive(full, ctx, out, out_len, out_cap, depth + 1)) {
                            free(full); free(base); free(src); return 0;
                        }
                        free(full);
                    }
                }
                continue;
            } else if (strncmp(q, "pragma", 6) == 0) {
                q += 6; q = skip_ws(q, line_end);
                if (active && strncmp(q, "once", 4) == 0) {
                    list_add_unique(&ctx->once_files, &ctx->once_count, &ctx->once_cap, path);
                }
                continue;
            } else if (strncmp(q, "ifndef", 6) == 0) {
                q += 6; q = skip_ws(q, line_end);
                char id[128]; parse_ident(&q, line_end, id, sizeof(id));
                int cond = !list_contains(ctx->macros, ctx->macro_count, id);
                if (sp < 64) {
                    active_stack[sp] = active && cond;
                    cond_stack[sp] = cond;
                    sp++;
                }
                continue;
            } else if (strncmp(q, "ifdef", 5) == 0) {
                q += 5; q = skip_ws(q, line_end);
                char id[128]; parse_ident(&q, line_end, id, sizeof(id));
                int cond = list_contains(ctx->macros, ctx->macro_count, id);
                if (sp < 64) {
                    active_stack[sp] = active && cond;
                    cond_stack[sp] = cond;
                    sp++;
                }
                continue;
            } else if (strncmp(q, "else", 4) == 0) {
                if (sp > 1) {
                    int parent_active = active_stack[sp - 2];
                    int original_cond = cond_stack[sp - 1];
                    active_stack[sp - 1] = parent_active && !original_cond;
                    cond_stack[sp - 1] = 1;
                }
                continue;
            } else if (strncmp(q, "define", 6) == 0) {
                q += 6; q = skip_ws(q, line_end);
                if (active) {
                    char id[128]; parse_ident(&q, line_end, id, sizeof(id));
                    if (id[0]) list_add_unique(&ctx->macros, &ctx->macro_count, &ctx->macro_cap, id);
                }
                continue;
            } else if (strncmp(q, "endif", 5) == 0) {
                if (sp > 1) sp--;
                continue;
            }
        }
        if (active) {
            size_t line_len = (size_t)(line_end - line_start);
            append_buf(out, out_len, out_cap, line_start, line_len);
            append_buf(out, out_len, out_cap, "\n", 1);
        }
    }

    free(base);
    free(src);
    return 1;
}

static char *preprocess_source_from_file(const char *input_path) {
    char *out = NULL;
    size_t out_len = 0, out_cap = 0;
    PreCtx ctx = {0};
    if (!preprocess_file_recursive(input_path, &ctx, &out, &out_len, &out_cap, 0)) {
        prec_free(&ctx);
        free(out);
        return NULL;
    }
    prec_free(&ctx);
    return out;
}

static int write_output(const char *path, IRProgram *ir, const char *target) {
    FILE *out = fopen(path, "w");
    if (!out) return 1;
    if (!emit_target(ir, out, target)) {
        fclose(out);
        return 1;
    }
    fclose(out);
    return 0;
}

static int has_supported_input_extension(const char *path) {
    size_t n = strlen(path);
    if (n >= 3 && strcmp(path + n - 3, ".cb") == 0) return 1;
    if (n >= 2 && strcmp(path + n - 2, ".h") == 0) return 1;
    return 0;
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
        fprintf(stderr, "Usage: gee <input.cb|input.h> [output.s]\n");
        return 1;
    }

    const char *in_path = argv[1];
    if (!has_supported_input_extension(in_path)) {
        fprintf(stderr, "error: input file must use .cb or .h extension\n");
        return 1;
    }
    char *auto_out = default_output_path(in_path);
    const char *out_path = (argc > 2) ? argv[2] : (auto_out ? auto_out : "out.s");
    const char *target = getenv("GEE_TARGET");
    if (!target || !*target) target = "x86-64";

    char *expanded_src = preprocess_source_from_file(in_path);
    if (!expanded_src) {
        fprintf(stderr, "Failed to preprocess input '%s'\n", in_path);
        free(auto_out);
        return 1;
    }

    Parser p = {0};
    AstNode *program = parse_program(&p, expanded_src);

    if (p.had_error) {
        fprintf(stderr, "Compilation failed due to parse/type errors.\n");
        /* NOTE: bootstrap mode intentionally avoids teardown because front-end
         * ownership is still being stabilized during self-hosting migration. */
        free(expanded_src);
        free(auto_out);
        return 1;
    }

    IRProgram ir;
    generate_ir(program, &ir);
    optimize_ir(&ir);

    if (write_output(out_path, &ir, target) != 0) {
        fprintf(stderr, "Failed to write assembly output '%s' for target '%s'\n", out_path, target);
        ir_free(&ir);
        free(expanded_src);
        free(auto_out);
        return 1;
    }

    printf("GEE: compiled %s -> %s\n", in_path, out_path);

    ir_free(&ir);
    (void)program;
    (void)p;
    free(expanded_src);
    free(auto_out);
    return 0;
}
