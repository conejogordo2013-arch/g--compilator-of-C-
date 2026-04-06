#include "codegen.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ir_emit(IRProgram *ir, IROp op, int a, int b, int c, const char *text) {
    if (ir->count == ir->capacity) {
        ir->capacity = ir->capacity ? ir->capacity * 2 : 256;
        ir->insts = (IRInst *)realloc(ir->insts, ir->capacity * sizeof(IRInst));
    }
    IRInst *i = &ir->insts[ir->count++];
    i->op = op;
    i->a = a;
    i->b = b;
    i->c = c;
    snprintf(i->text, sizeof(i->text), "%s", text ? text : "");
}

void ir_init(IRProgram *ir) { memset(ir, 0, sizeof(*ir)); }
void ir_free(IRProgram *ir) { free(ir->insts); memset(ir, 0, sizeof(*ir)); }

static int fresh_temp(IRProgram *ir) { return ir->temp_counter++; }
static int fresh_label(IRProgram *ir) { return ir->label_counter++; }
static AstNode *current_program = NULL;

static AstNode *find_function_decl(const char *name) {
    if (!current_program) return NULL;
    for (size_t i = 0; i < current_program->as.program.decls.count; ++i) {
        AstNode *d = current_program->as.program.decls.items[i];
        if (d->kind == AST_FUNCTION && strcmp(d->as.function.name, name) == 0) return d;
    }
    return NULL;
}

static int gen_expr(AstNode *n, IRProgram *ir);
static void gen_stmt(AstNode *n, IRProgram *ir);

static int gen_expr(AstNode *n, IRProgram *ir) {
    if (!n) return -1;
    switch (n->kind) {
        case AST_INT_LITERAL: {
            int t = fresh_temp(ir);
            char txt[64];
            snprintf(txt, sizeof(txt), "t%d = %lld", t, n->as.int_lit.value);
            ir_emit(ir, IR_MOVI, t, (int)n->as.int_lit.value, 0, txt);
            return t;
        }
        case AST_STRING_LITERAL: {
            int t = fresh_temp(ir);
            ir_emit(ir, IR_STRING, t, 0, 0, n->as.str_lit.value);
            return t;
        }
        case AST_IDENTIFIER: {
            int t = fresh_temp(ir);
            char txt[64];
            snprintf(txt, sizeof(txt), "t%d = load %s", t, n->as.ident_expr.name);
            ir_emit(ir, IR_LOAD, t, 0, 0, txt);
            return t;
        }
        case AST_ASSIGN: {
            int v = gen_expr(n->as.assign_expr.value, ir);
            char txt[64];
            snprintf(txt, sizeof(txt), "store %s, t%d", n->as.assign_expr.target, v);
            ir_emit(ir, IR_STORE, v, 0, 0, txt);
            return v;
        }
        case AST_PTR_ASSIGN: {
            int ptr = gen_expr(n->as.ptr_assign_expr.ptr, ir);
            int val = gen_expr(n->as.ptr_assign_expr.value, ir);
            ir_emit(ir, IR_STORE_IND, ptr, val, 0, "store_ind");
            return val;
        }
        case AST_FIELD: {
            int base_ptr = -1;
            if (n->as.field_expr.through_ptr) {
                base_ptr = gen_expr(n->as.field_expr.base, ir);
            } else if (n->as.field_expr.base && n->as.field_expr.base->kind == AST_IDENTIFIER) {
                base_ptr = fresh_temp(ir);
                ir_emit(ir, IR_ADDR, base_ptr, 0, 0, n->as.field_expr.base->as.ident_expr.name);
            } else {
                base_ptr = gen_expr(n->as.field_expr.base, ir);
            }
            int off = fresh_temp(ir);
            ir_emit(ir, IR_MOVI, off, n->as.field_expr.offset, 0, "field_off");
            int addr = fresh_temp(ir);
            ir_emit(ir, IR_ADD, addr, base_ptr, off, "field_addr");
            int out = fresh_temp(ir);
            ir_emit(ir, IR_LOAD_IND, out, addr, 0, "field_load");
            return out;
        }
        case AST_FIELD_ASSIGN: {
            int base_ptr = -1;
            if (n->as.field_assign_expr.through_ptr) {
                base_ptr = gen_expr(n->as.field_assign_expr.base, ir);
            } else if (n->as.field_assign_expr.base && n->as.field_assign_expr.base->kind == AST_IDENTIFIER) {
                base_ptr = fresh_temp(ir);
                ir_emit(ir, IR_ADDR, base_ptr, 0, 0, n->as.field_assign_expr.base->as.ident_expr.name);
            } else {
                base_ptr = gen_expr(n->as.field_assign_expr.base, ir);
            }
            int off = fresh_temp(ir);
            ir_emit(ir, IR_MOVI, off, n->as.field_assign_expr.offset, 0, "field_off");
            int addr = fresh_temp(ir);
            ir_emit(ir, IR_ADD, addr, base_ptr, off, "field_addr");
            int val = gen_expr(n->as.field_assign_expr.value, ir);
            ir_emit(ir, IR_STORE_IND, addr, val, 0, "field_store");
            return val;
        }
        case AST_UNARY: {
            int rhs = gen_expr(n->as.unary_expr.expr, ir);
            if (strcmp(n->as.unary_expr.op, "&") == 0 && n->as.unary_expr.expr &&
                n->as.unary_expr.expr->kind == AST_IDENTIFIER) {
                int out = fresh_temp(ir);
                ir_emit(ir, IR_ADDR, out, 0, 0, n->as.unary_expr.expr->as.ident_expr.name);
                return out;
            }
            if (strcmp(n->as.unary_expr.op, "*") == 0) {
                int out = fresh_temp(ir);
                ir_emit(ir, IR_LOAD_IND, out, rhs, 0, "load_ind");
                return out;
            }
            if (strcmp(n->as.unary_expr.op, "-") == 0) {
                int z = fresh_temp(ir);
                ir_emit(ir, IR_MOVI, z, 0, 0, "zero");
                int out = fresh_temp(ir);
                ir_emit(ir, IR_SUB, out, z, rhs, "neg");
                return out;
            }
            if (strcmp(n->as.unary_expr.op, "!") == 0) {
                int z = fresh_temp(ir);
                ir_emit(ir, IR_MOVI, z, 0, 0, "zero");
                int out = fresh_temp(ir);
                ir_emit(ir, IR_CMP, rhs, z, out, "==");
                return out;
            }
            return rhs;
        }
        case AST_BINARY: {
            int l = gen_expr(n->as.binary_expr.lhs, ir);
            int r = gen_expr(n->as.binary_expr.rhs, ir);
            int out = fresh_temp(ir);
            if (strcmp(n->as.binary_expr.op, "+") == 0) ir_emit(ir, IR_ADD, out, l, r, "add");
            else if (strcmp(n->as.binary_expr.op, "-") == 0) ir_emit(ir, IR_SUB, out, l, r, "sub");
            else if (strcmp(n->as.binary_expr.op, "*") == 0) ir_emit(ir, IR_MUL, out, l, r, "mul");
            else if (strcmp(n->as.binary_expr.op, "/") == 0) ir_emit(ir, IR_DIV, out, l, r, "div");
            else {
                /* Comparison lowered to cmp + synthetic set; backend will convert using cmp/jcc. */
                ir_emit(ir, IR_CMP, l, r, out, n->as.binary_expr.op);
            }
            return out;
        }
        case AST_CALL: {
            AstNode *fd = find_function_decl(n->as.call_expr.name);
            if (fd && fd->as.function.can_inline && fd->as.function.params.count == 0 &&
                fd->as.function.body && fd->as.function.body->kind == AST_BLOCK &&
                fd->as.function.body->as.block.statements.count == 1) {
                AstNode *ret = fd->as.function.body->as.block.statements.items[0];
                if (ret->kind == AST_RETURN && ret->as.return_stmt.expr) {
                    return gen_expr(ret->as.return_stmt.expr, ir);
                }
            }
            int argtemps[16] = {0};
            size_t argc = n->as.call_expr.args.count;
            if (argc > 16) argc = 16;
            for (size_t i = 0; i < argc; ++i) {
                argtemps[i] = gen_expr(n->as.call_expr.args.items[i], ir);
            }
            int out = fresh_temp(ir);
            char desc[96] = {0};
            snprintf(desc, sizeof(desc), "%s", n->as.call_expr.name);
            for (size_t i = 0; i < argc; ++i) {
                char tmp[16];
                snprintf(tmp, sizeof(tmp), "|%d", argtemps[i]);
                strncat(desc, tmp, sizeof(desc) - strlen(desc) - 1);
            }
            ir_emit(ir, IR_CALL, out, (int)argc, 0, desc);
            return out;
        }
        default:
            return -1;
    }
}

static void gen_block(AstNode *b, IRProgram *ir) {
    for (size_t i = 0; i < b->as.block.statements.count; ++i) gen_stmt(b->as.block.statements.items[i], ir);
}

static void gen_stmt(AstNode *n, IRProgram *ir) {
    if (!n) return;
    switch (n->kind) {
        case AST_BLOCK:
            gen_block(n, ir);
            break;
        case AST_VAR_DECL:
            if (n->as.var_decl.init) {
                int t = gen_expr(n->as.var_decl.init, ir);
                char txt[64];
                snprintf(txt, sizeof(txt), "store %s, t%d", n->as.var_decl.name, t);
                ir_emit(ir, IR_STORE, t, 0, 0, txt);
            }
            break;
        case AST_EXPR_STMT:
            (void)gen_expr(n->as.expr_stmt.expr, ir);
            break;
        case AST_IF: {
            int cond = gen_expr(n->as.if_stmt.cond, ir);
            int lbl_else = fresh_label(ir);
            int lbl_end = fresh_label(ir);
            char jtxt[64];
            snprintf(jtxt, sizeof(jtxt), "if_false t%d -> L%d", cond, lbl_else);
            ir_emit(ir, IR_JE, cond, lbl_else, 0, jtxt);
            gen_stmt(n->as.if_stmt.then_branch, ir);
            char jend[32];
            snprintf(jend, sizeof(jend), "L%d", lbl_end);
            ir_emit(ir, IR_JMP, lbl_end, 0, 0, jend);
            char le[32]; snprintf(le, sizeof(le), "L%d", lbl_else);
            ir_emit(ir, IR_LABEL, lbl_else, 0, 0, le);
            gen_stmt(n->as.if_stmt.else_branch, ir);
            char ln[32]; snprintf(ln, sizeof(ln), "L%d", lbl_end);
            ir_emit(ir, IR_LABEL, lbl_end, 0, 0, ln);
            break;
        }
        case AST_WHILE: {
            int lbl_cond = fresh_label(ir);
            int lbl_body = fresh_label(ir);
            int lbl_end = fresh_label(ir);
            char lc[32]; snprintf(lc, sizeof(lc), "L%d", lbl_cond);
            ir_emit(ir, IR_LABEL, lbl_cond, 0, 0, lc);
            int c = gen_expr(n->as.while_stmt.cond, ir);
            char jf[64]; snprintf(jf, sizeof(jf), "if_false t%d -> L%d", c, lbl_end);
            ir_emit(ir, IR_JE, c, lbl_end, 0, jf);
            char lb[32]; snprintf(lb, sizeof(lb), "L%d", lbl_body);
            ir_emit(ir, IR_LABEL, lbl_body, 0, 0, lb);
            gen_stmt(n->as.while_stmt.body, ir);
            ir_emit(ir, IR_JMP, lbl_cond, 0, 0, lc);
            char le[32]; snprintf(le, sizeof(le), "L%d", lbl_end);
            ir_emit(ir, IR_LABEL, lbl_end, 0, 0, le);
            break;
        }
        case AST_FOR: {
            if (n->as.for_stmt.init) gen_expr(n->as.for_stmt.init, ir);
            int lbl_cond = fresh_label(ir), lbl_body = fresh_label(ir), lbl_end = fresh_label(ir);
            char lc[32]; snprintf(lc, sizeof(lc), "L%d", lbl_cond);
            ir_emit(ir, IR_LABEL, lbl_cond, 0, 0, lc);
            if (n->as.for_stmt.cond) {
                int c = gen_expr(n->as.for_stmt.cond, ir);
                char jf[64]; snprintf(jf, sizeof(jf), "if_false t%d -> L%d", c, lbl_end);
                ir_emit(ir, IR_JE, c, lbl_end, 0, jf);
            }
            char lb[32]; snprintf(lb, sizeof(lb), "L%d", lbl_body);
            ir_emit(ir, IR_LABEL, lbl_body, 0, 0, lb);
            gen_stmt(n->as.for_stmt.body, ir);
            if (n->as.for_stmt.post) gen_expr(n->as.for_stmt.post, ir);
            ir_emit(ir, IR_JMP, lbl_cond, 0, 0, lc);
            char le[32]; snprintf(le, sizeof(le), "L%d", lbl_end);
            ir_emit(ir, IR_LABEL, lbl_end, 0, 0, le);
            break;
        }
        case AST_RETURN:
            if (n->as.return_stmt.expr) {
                int t = gen_expr(n->as.return_stmt.expr, ir);
                ir_emit(ir, IR_RET, t, 0, 0, "ret value");
            } else {
                ir_emit(ir, IR_RET, -1, 0, 0, "ret");
            }
            break;
        case AST_SWITCH:
            /* Simplified lowering path for switch. */
            if (n->as.switch_stmt.expr) (void)gen_expr(n->as.switch_stmt.expr, ir);
            for (size_t i = 0; i < n->as.switch_stmt.cases.count; ++i) {
                gen_stmt(n->as.switch_stmt.cases.items[i]->as.case_stmt.body, ir);
            }
            gen_stmt(n->as.switch_stmt.default_case, ir);
            break;
        default:
            break;
    }
}

void generate_ir(AstNode *program, IRProgram *ir) {
    ir_init(ir);
    current_program = program;

    for (size_t i = 0; i < program->as.program.decls.count; ++i) {
        AstNode *d = program->as.program.decls.items[i];
        if (d->kind == AST_VAR_DECL) {
            int init = 0;
            if (d->as.var_decl.init && d->as.var_decl.init->kind == AST_INT_LITERAL) {
                init = (int)d->as.var_decl.init->as.int_lit.value;
            }
            ir_emit(ir, IR_GLOBAL, init, 0, 0, d->as.var_decl.name);
        }
    }

    for (size_t i = 0; i < program->as.program.decls.count; ++i) {
        AstNode *d = program->as.program.decls.items[i];
        if (d->kind == AST_FUNCTION) {
            /* Cross-module linking in bootstrap mode requires emitting all symbols. */
            char lbl[64];
            snprintf(lbl, sizeof(lbl), "fn_%s", d->as.function.name);
            ir_emit(ir, IR_LABEL, 0, 0, 0, lbl);
            /* Placeholder stack frame handling. */
            ir_emit(ir, IR_NOP, 0, 0, 0, "prologue: push rbp; mov rbp, rsp; sub rsp, <locals>");
            for (size_t pidx = 0; pidx < d->as.function.params.count; ++pidx) {
                AstNode *param = d->as.function.params.items[pidx];
                ir_emit(ir, IR_PARAM, (int)pidx, 0, 0, param->as.param.name);
            }
            gen_stmt(d->as.function.body, ir);
            ir_emit(ir, IR_NOP, 0, 0, 0, "epilogue: mov rsp, rbp; pop rbp");
            ir_emit(ir, IR_RET, -1, 0, 0, "ret");
        }
    }
    current_program = NULL;
}

typedef struct VarSlot {
    char name[64];
    int offset;
} VarSlot;

static int temp_offset(int t) {
    return 8 * (t + 1);
}

static int find_slot(VarSlot *slots, int count, const char *name) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(slots[i].name, name) == 0) return i;
    }
    return -1;
}

static bool is_global_symbol(IRProgram *ir, const char *name) {
    for (size_t i = 0; i < ir->count; ++i) {
        if (ir->insts[i].op == IR_GLOBAL && strcmp(ir->insts[i].text, name) == 0) return true;
    }
    return false;
}

static bool parse_store_text(const char *text, char *name, size_t name_cap) {
    const char *p = strstr(text, "store ");
    if (!p) return false;
    p += 6;
    size_t n = 0;
    while (p[n] && p[n] != ',' && !isspace((unsigned char)p[n]) && n + 1 < name_cap) n++;
    if (n == 0) return false;
    memcpy(name, p, n);
    name[n] = '\0';
    return true;
}

static bool parse_load_text(const char *text, char *name, size_t name_cap) {
    const char *p = strstr(text, "load ");
    if (!p) return false;
    p += 5;
    size_t n = 0;
    while (p[n] && !isspace((unsigned char)p[n]) && n + 1 < name_cap) n++;
    if (n == 0) return false;
    memcpy(name, p, n);
    name[n] = '\0';
    return true;
}

static void emit_escaped_c_string(FILE *out, const char *s) {
    fputc('"', out);
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        if (*p == '\\' || *p == '"') {
            fputc('\\', out);
            fputc(*p, out);
        } else if (*p == '\n') {
            fputs("\\n", out);
        } else if (*p == '\t') {
            fputs("\\t", out);
        } else if (*p < 32 || *p > 126) {
            fprintf(out, "\\x%02x", *p);
        } else {
            fputc(*p, out);
        }
    }
    fputc('"', out);
}

static int parse_call_descriptor(const char *desc, char *name, size_t name_cap, int *args, int max_args) {
    if (!desc || !*desc) return 0;
    size_t i = 0;
    while (desc[i] && desc[i] != '|' && i + 1 < name_cap) {
        name[i] = desc[i];
        i++;
    }
    name[i] = '\0';

    int argc = 0;
    const char *p = desc + i;
    while (*p == '|' && argc < max_args) {
        p++;
        char *end = NULL;
        long v = strtol(p, &end, 10);
        if (end == p) break;
        args[argc++] = (int)v;
        p = end;
    }
    return argc;
}

void emit_x86_64(IRProgram *ir, FILE *out) {
    enum { STACK_FRAME_SIZE = 16384 };

    fprintf(out, ".intel_syntax noprefix\n");
    bool has_globals = false;
    for (size_t i = 0; i < ir->count; ++i) {
        if (ir->insts[i].op == IR_GLOBAL) {
            has_globals = true;
            break;
        }
    }
    if (has_globals) {
        fprintf(out, ".data\n");
        for (size_t i = 0; i < ir->count; ++i) {
            IRInst *in = &ir->insts[i];
            if (in->op == IR_GLOBAL) {
                fprintf(out, ".globl %s\n", in->text);
                fprintf(out, "%s:\n", in->text);
                fprintf(out, "    .quad %d\n", in->a);
            }
        }
    }
    fprintf(out, ".text\n");

    VarSlot local_slots[256];
    int local_count = 0;
    bool in_function = false;

    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        switch (in->op) {
            case IR_LABEL:
                if (strncmp(in->text, "fn_", 3) == 0) {
                    local_count = 0;
                    in_function = true;
                    fprintf(out, ".globl %s\n%s:\n", in->text + 3, in->text + 3);
                    fprintf(out, "    push rbp\n");
                    fprintf(out, "    mov rbp, rsp\n");
                    fprintf(out, "    sub rsp, %d\n", STACK_FRAME_SIZE);
                } else fprintf(out, "%s:\n", in->text);
                break;
            case IR_NOP:
                /* Prologue/epilogue are now emitted concretely per function. */
                break;
            case IR_MOVI:
                fprintf(out, "    mov rax, %d\n", in->b);
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_ADD:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    add rax, rbx\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_SUB:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    sub rax, rbx\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_MUL:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    imul rax, rbx\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_DIV:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    cqo\n    idiv rbx\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_CMP:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->a));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                if (strcmp(in->text, "&&") == 0) {
                    fprintf(out, "    cmp rax, 0\n");
                    fprintf(out, "    setne al\n");
                    fprintf(out, "    cmp rbx, 0\n");
                    fprintf(out, "    setne bl\n");
                    fprintf(out, "    and al, bl\n");
                    fprintf(out, "    movzx rax, al\n");
                } else if (strcmp(in->text, "||") == 0) {
                    fprintf(out, "    cmp rax, 0\n");
                    fprintf(out, "    setne al\n");
                    fprintf(out, "    cmp rbx, 0\n");
                    fprintf(out, "    setne bl\n");
                    fprintf(out, "    or al, bl\n");
                    fprintf(out, "    movzx rax, al\n");
                } else {
                    fprintf(out, "    cmp rax, rbx\n");
                    if (strcmp(in->text, "==") == 0) fprintf(out, "    sete al\n");
                    else if (strcmp(in->text, "!=") == 0) fprintf(out, "    setne al\n");
                    else if (strcmp(in->text, "<") == 0) fprintf(out, "    setl al\n");
                    else if (strcmp(in->text, "<=") == 0) fprintf(out, "    setle al\n");
                    else if (strcmp(in->text, ">") == 0) fprintf(out, "    setg al\n");
                    else if (strcmp(in->text, ">=") == 0) fprintf(out, "    setge al\n");
                    else fprintf(out, "    setne al\n");
                    fprintf(out, "    movzx rax, al\n");
                }
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->c));
                break;
            case IR_JMP:
                fprintf(out, "    jmp L%d\n", in->a);
                break;
            case IR_JE:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->a));
                fprintf(out, "    cmp rax, 0\n");
                fprintf(out, "    je L%d\n", in->b);
                break;
            case IR_JNE:
                fprintf(out, "    jne L%d\n", in->b);
                break;
            case IR_CALL:
                {
                    static const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                    int args[16] = {0};
                    char callee[64] = {0};
                    int argc = parse_call_descriptor(in->text, callee, sizeof(callee), args, 16);
                    int stack_args = (argc > 6) ? (argc - 6) : 0;
                    int align_pad = (stack_args % 2) ? 8 : 0;
                    if (align_pad) fprintf(out, "    sub rsp, %d\n", align_pad);
                    for (int ai = argc - 1; ai >= 6; --ai) {
                        fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(args[ai]));
                        fprintf(out, "    push rax\n");
                    }
                    for (int ai = 0; ai < argc && ai < 6; ++ai) {
                        fprintf(out, "    mov %s, QWORD PTR [rbp-%d]\n", arg_regs[ai], temp_offset(args[ai]));
                    }
                    fprintf(out, "    xor eax, eax\n");
                    fprintf(out, "    call %s\n", callee[0] ? callee : in->text);
                    if (stack_args > 0 || align_pad) fprintf(out, "    add rsp, %d\n", stack_args * 8 + align_pad);
                }
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_STORE:
                {
                    char name[64] = {0};
                    if (!parse_store_text(in->text, name, sizeof(name))) {
                        fprintf(out, "    # malformed store: %s\n", in->text);
                        break;
                    }
                    fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->a));
                    if (is_global_symbol(ir, name)) {
                        fprintf(out, "    mov QWORD PTR %s[rip], rax\n", name);
                    } else {
                        int idx = find_slot(local_slots, local_count, name);
                        if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                            idx = local_count++;
                            snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", name);
                            local_slots[idx].offset = 4096 + idx * 8;
                        }
                        if (idx >= 0) fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", local_slots[idx].offset);
                    }
                }
                break;
            case IR_LOAD:
                {
                    char name[64] = {0};
                    if (!parse_load_text(in->text, name, sizeof(name))) {
                        fprintf(out, "    # malformed load: %s\n", in->text);
                        break;
                    }
                    if (is_global_symbol(ir, name)) {
                        fprintf(out, "    mov rax, QWORD PTR %s[rip]\n", name);
                    } else {
                        int idx = find_slot(local_slots, local_count, name);
                        if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                            idx = local_count++;
                            snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", name);
                            local_slots[idx].offset = 4096 + idx * 8;
                        }
                        if (idx >= 0) fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", local_slots[idx].offset);
                    }
                    fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                }
                break;
            case IR_RET:
                if (in->a >= 0) fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->a));
                if (in_function) {
                    fprintf(out, "    leave\n");
                }
                fprintf(out, "    ret\n");
                break;
            case IR_GLOBAL:
                /* Emitted in data prelude above. */
                break;
            case IR_PARAM:
                {
                    static const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
                    int idx = find_slot(local_slots, local_count, in->text);
                    if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                        idx = local_count++;
                        snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", in->text);
                        local_slots[idx].offset = 4096 + idx * 8;
                    }
                    if (idx >= 0 && in->a >= 0) {
                        if (in->a < 6) {
                            fprintf(out, "    mov QWORD PTR [rbp-%d], %s\n", local_slots[idx].offset, arg_regs[in->a]);
                        } else {
                            int stack_off = 16 + (in->a - 6) * 8;
                            fprintf(out, "    mov rax, QWORD PTR [rbp+%d]\n", stack_off);
                            fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", local_slots[idx].offset);
                        }
                    }
                }
                break;
            case IR_ADDR:
                if (is_global_symbol(ir, in->text)) {
                    fprintf(out, "    lea rax, %s[rip]\n", in->text);
                } else {
                    int idx = find_slot(local_slots, local_count, in->text);
                    if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                        idx = local_count++;
                        snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", in->text);
                        local_slots[idx].offset = 4096 + idx * 8;
                    }
                    if (idx >= 0) fprintf(out, "    lea rax, [rbp-%d]\n", local_slots[idx].offset);
                }
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_LOAD_IND:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rax, QWORD PTR [rax]\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_STORE_IND:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->a));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov QWORD PTR [rax], rbx\n");
                break;
            case IR_STRING:
                fprintf(out, "    lea rax, .LC%zu[rip]\n", i);
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
        }
    }
}
