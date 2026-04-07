#include "codegen.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_ir_line = 0;

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
    i->line = g_ir_line;
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
    g_ir_line = n->line;
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
            if (strcmp(n->as.unary_expr.op, "~") == 0) {
                int all1 = fresh_temp(ir);
                ir_emit(ir, IR_MOVI, all1, -1, 0, "all1");
                int out = fresh_temp(ir);
                ir_emit(ir, IR_XOR, out, rhs, all1, "bnot");
                return out;
            }
            return rhs;
        }
        case AST_BINARY: {
            if (strcmp(n->as.binary_expr.op, "&&") == 0) {
                int out = fresh_temp(ir);
                int lbl_false = fresh_label(ir);
                int l = gen_expr(n->as.binary_expr.lhs, ir);
                ir_emit(ir, IR_MOVI, out, 0, 0, "false");
                ir_emit(ir, IR_JE, l, lbl_false, 0, "sc_and_lhs_false");
                int r = gen_expr(n->as.binary_expr.rhs, ir);
                ir_emit(ir, IR_JE, r, lbl_false, 0, "sc_and_rhs_false");
                ir_emit(ir, IR_MOVI, out, 1, 0, "true");
                char lf[32]; snprintf(lf, sizeof(lf), "L%d", lbl_false);
                ir_emit(ir, IR_LABEL, lbl_false, 0, 0, lf);
                return out;
            }
            if (strcmp(n->as.binary_expr.op, "||") == 0) {
                int out = fresh_temp(ir);
                int lbl_eval_rhs = fresh_label(ir);
                int lbl_false = fresh_label(ir);
                int lbl_end = fresh_label(ir);
                int l = gen_expr(n->as.binary_expr.lhs, ir);
                ir_emit(ir, IR_MOVI, out, 1, 0, "true");
                ir_emit(ir, IR_JE, l, lbl_eval_rhs, 0, "sc_or_eval_rhs");
                ir_emit(ir, IR_JMP, lbl_end, 0, 0, "sc_or_lhs_true");
                char le[32]; snprintf(le, sizeof(le), "L%d", lbl_eval_rhs);
                ir_emit(ir, IR_LABEL, lbl_eval_rhs, 0, 0, le);
                int r = gen_expr(n->as.binary_expr.rhs, ir);
                ir_emit(ir, IR_JE, r, lbl_false, 0, "sc_or_rhs_false");
                ir_emit(ir, IR_JMP, lbl_end, 0, 0, "sc_or_rhs_true");
                char lf[32]; snprintf(lf, sizeof(lf), "L%d", lbl_false);
                ir_emit(ir, IR_LABEL, lbl_false, 0, 0, lf);
                ir_emit(ir, IR_MOVI, out, 0, 0, "false");
                char ln[32]; snprintf(ln, sizeof(ln), "L%d", lbl_end);
                ir_emit(ir, IR_LABEL, lbl_end, 0, 0, ln);
                return out;
            }

            int l = gen_expr(n->as.binary_expr.lhs, ir);
            int r = gen_expr(n->as.binary_expr.rhs, ir);
            int out = fresh_temp(ir);
            if (strcmp(n->as.binary_expr.op, "+") == 0) ir_emit(ir, IR_ADD, out, l, r, "add");
            else if (strcmp(n->as.binary_expr.op, "-") == 0) ir_emit(ir, IR_SUB, out, l, r, "sub");
            else if (strcmp(n->as.binary_expr.op, "*") == 0) ir_emit(ir, IR_MUL, out, l, r, "mul");
            else if (strcmp(n->as.binary_expr.op, "/") == 0) ir_emit(ir, IR_DIV, out, l, r, "div");
            else if (strcmp(n->as.binary_expr.op, "%") == 0) ir_emit(ir, IR_MOD, out, l, r, "mod");
            else if (strcmp(n->as.binary_expr.op, "&") == 0) ir_emit(ir, IR_AND, out, l, r, "and");
            else if (strcmp(n->as.binary_expr.op, "|") == 0) ir_emit(ir, IR_OR, out, l, r, "or");
            else if (strcmp(n->as.binary_expr.op, "^") == 0) ir_emit(ir, IR_XOR, out, l, r, "xor");
            else if (strcmp(n->as.binary_expr.op, "<<") == 0) ir_emit(ir, IR_SHL, out, l, r, "shl");
            else if (strcmp(n->as.binary_expr.op, ">>") == 0) ir_emit(ir, IR_SHR, out, l, r, "shr");
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
    g_ir_line = n->line;
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

void optimize_ir(IRProgram *ir) {
    if (!ir || !ir->insts || ir->count == 0) return;

    enum { MAX_TEMPS = 16384 };
    bool known[MAX_TEMPS];
    long long value[MAX_TEMPS];
    memset(known, 0, sizeof(known));
    memset(value, 0, sizeof(value));

    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        if (in->op == IR_MOVI && in->a >= 0 && in->a < MAX_TEMPS) {
            known[in->a] = true;
            value[in->a] = in->b;
            continue;
        }

        if ((in->op == IR_ADD || in->op == IR_SUB || in->op == IR_MUL || in->op == IR_DIV || in->op == IR_MOD ||
             in->op == IR_AND || in->op == IR_OR || in->op == IR_XOR || in->op == IR_SHL || in->op == IR_SHR) &&
            in->a >= 0 && in->a < MAX_TEMPS &&
            in->b >= 0 && in->b < MAX_TEMPS &&
            in->c >= 0 && in->c < MAX_TEMPS &&
            known[in->b] && known[in->c]) {
            long long lhs = value[in->b], rhs = value[in->c], out = 0;
            if (in->op == IR_ADD) out = lhs + rhs;
            else if (in->op == IR_SUB) out = lhs - rhs;
            else if (in->op == IR_MUL) out = lhs * rhs;
            else if (in->op == IR_DIV) {
                if (rhs == 0) continue;
                out = lhs / rhs;
            } else if (in->op == IR_MOD) {
                if (rhs == 0) continue;
                out = lhs % rhs;
            } else if (in->op == IR_AND) {
                out = lhs & rhs;
            } else if (in->op == IR_OR) {
                out = lhs | rhs;
            } else if (in->op == IR_XOR) {
                out = lhs ^ rhs;
            } else if (in->op == IR_SHL) {
                out = lhs << rhs;
            } else if (in->op == IR_SHR) {
                out = lhs >> rhs;
            }
            in->op = IR_MOVI;
            in->b = (int)out;
            in->c = 0;
            snprintf(in->text, sizeof(in->text), "folded");
            known[in->a] = true;
            value[in->a] = out;
            continue;
        }

        if (in->op == IR_CMP &&
            in->c >= 0 && in->c < MAX_TEMPS &&
            in->a >= 0 && in->a < MAX_TEMPS &&
            in->b >= 0 && in->b < MAX_TEMPS &&
            known[in->a] && known[in->b]) {
            long long lhs = value[in->a], rhs = value[in->b];
            int out = 0;
            if (strcmp(in->text, "==") == 0) out = (lhs == rhs);
            else if (strcmp(in->text, "!=") == 0) out = (lhs != rhs);
            else if (strcmp(in->text, "<") == 0) out = (lhs < rhs);
            else if (strcmp(in->text, "<=") == 0) out = (lhs <= rhs);
            else if (strcmp(in->text, ">") == 0) out = (lhs > rhs);
            else if (strcmp(in->text, ">=") == 0) out = (lhs >= rhs);
            else if (strcmp(in->text, "&&") == 0) out = ((lhs != 0) && (rhs != 0));
            else if (strcmp(in->text, "||") == 0) out = ((lhs != 0) || (rhs != 0));
            else continue;
            in->op = IR_MOVI;
            in->a = in->c;
            in->b = out;
            in->c = 0;
            snprintf(in->text, sizeof(in->text), "folded_cmp");
            known[in->a] = true;
            value[in->a] = out;
            continue;
        }

        if (in->a >= 0 && in->a < MAX_TEMPS) known[in->a] = false;
    }

    IRInst *out = (IRInst *)malloc(ir->capacity * sizeof(IRInst));
    size_t out_count = 0;
    bool in_dead_zone = false;
    for (size_t i = 0; i < ir->count; ++i) {
        IRInst in = ir->insts[i];
        if (in.op == IR_LABEL) in_dead_zone = false;
        if (in_dead_zone && in.op != IR_LABEL) continue;
        if (in.op == IR_NOP) continue;
        out[out_count++] = in;
        if (in.op == IR_RET || in.op == IR_JMP) in_dead_zone = true;
    }
    memcpy(ir->insts, out, out_count * sizeof(IRInst));
    ir->count = out_count;
    free(out);
}

typedef struct VarSlot {
    char name[64];
    int offset;
} VarSlot;

static int temp_offset(int t) {
    return 8 * (t + 1);
}

static int parse_call_descriptor(const char *desc, char *name, size_t name_cap, int *args, int max_args);

static int align_up16(int n) {
    return (n + 15) & ~15;
}

static void compute_frame_layout(IRProgram *ir, int *frame_size, int *locals_base) {
    int max_temp = -1;
    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        switch (in->op) {
            case IR_MOVI:
            case IR_LOAD:
            case IR_ADDR:
            case IR_STRING:
                if (in->a > max_temp) max_temp = in->a;
                break;
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV:
            case IR_MOD:
            case IR_AND:
            case IR_OR:
            case IR_XOR:
            case IR_SHL:
            case IR_SHR:
            case IR_CMP:
            case IR_LOAD_IND:
            case IR_STORE_IND:
                if (in->a > max_temp) max_temp = in->a;
                if (in->b > max_temp) max_temp = in->b;
                if (in->c > max_temp) max_temp = in->c;
                break;
            case IR_JE:
            case IR_STORE:
            case IR_RET:
                if (in->a > max_temp) max_temp = in->a;
                break;
            case IR_CALL: {
                if (in->a > max_temp) max_temp = in->a;
                int args[16] = {0};
                char callee[64] = {0};
                int argc = parse_call_descriptor(in->text, callee, sizeof(callee), args, 16);
                for (int ai = 0; ai < argc; ++ai) {
                    if (args[ai] > max_temp) max_temp = args[ai];
                }
                break;
            }
            default:
                break;
        }
    }
    int temp_area = temp_offset(max_temp + 1);
    int base = align_up16(temp_area + 64);
    int frame = align_up16(base + 256 * 8 + 64);
    if (locals_base) *locals_base = base;
    if (frame_size) *frame_size = frame;
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
    int STACK_FRAME_SIZE = 0;
    int LOCALS_BASE_OFFSET = 0;
    compute_frame_layout(ir, &STACK_FRAME_SIZE, &LOCALS_BASE_OFFSET);

    fprintf(out, ".intel_syntax noprefix\n");
    bool has_globals = false;
    bool has_strings = false;
    for (size_t i = 0; i < ir->count; ++i) {
        if (ir->insts[i].op == IR_GLOBAL) {
            has_globals = true;
            break;
        }
    }
    for (size_t i = 0; i < ir->count; ++i) {
        if (ir->insts[i].op == IR_STRING) {
            has_strings = true;
            break;
        }
    }
    if (has_strings) {
        fprintf(out, ".section .rodata\n");
        for (size_t i = 0; i < ir->count; ++i) {
            IRInst *in = &ir->insts[i];
            if (in->op == IR_STRING) {
                fprintf(out, ".LC%zu:\n", i);
                fprintf(out, "    .string ");
                emit_escaped_c_string(out, in->text);
                fprintf(out, "\n");
            }
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
    int known_const[65536] = {0};
    unsigned char has_known_const[65536] = {0};
    char cached_local_name[2][64] = {{0}};
    bool cached_local_valid[2] = {false, false};
    const char *cached_local_reg[2] = {"r10", "r11"};
    bool local_cache_enabled = true;
    bool debug_map = getenv("GEE_DEBUG_MAP") != NULL;

    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        if (in->op == IR_MOVI && in->a >= 0 && in->a < (int)(sizeof(known_const) / sizeof(known_const[0]))) {
            known_const[in->a] = in->b;
            has_known_const[in->a] = 1;
        }
    }

    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        if (debug_map && in->line > 0) {
            fprintf(out, "    # map src:%d ir:%s\n", in->line, in->text);
        }
        switch (in->op) {
            case IR_LABEL:
                if (strncmp(in->text, "fn_", 3) == 0) {
                    local_count = 0;
                    in_function = true;
                    cached_local_valid[0] = cached_local_valid[1] = false;
                    local_cache_enabled = true;
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
                if (in->b >= 0 && in->b < 65536 && has_known_const[in->b] &&
                    in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov rax, %d\n", known_const[in->b] + known_const[in->c]);
                } else if (in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                    fprintf(out, "    add rax, %d\n", known_const[in->c]);
                } else if (in->b >= 0 && in->b < 65536 && has_known_const[in->b]) {
                    fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                    fprintf(out, "    add rax, %d\n", known_const[in->b]);
                } else {
                    fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                    fprintf(out, "    add rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                }
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_SUB:
                if (in->b >= 0 && in->b < 65536 && has_known_const[in->b] &&
                    in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov rax, %d\n", known_const[in->b] - known_const[in->c]);
                } else if (in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                    fprintf(out, "    sub rax, %d\n", known_const[in->c]);
                } else if (in->b >= 0 && in->b < 65536 && has_known_const[in->b]) {
                    fprintf(out, "    mov rax, %d\n", known_const[in->b]);
                    fprintf(out, "    sub rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                } else {
                    fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                    fprintf(out, "    sub rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                }
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_MUL:
                if (in->b >= 0 && in->b < 65536 && has_known_const[in->b] &&
                    in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov rax, %d\n", known_const[in->b] * known_const[in->c]);
                } else if (in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    int k = known_const[in->c];
                    if (k == 0) fprintf(out, "    xor eax, eax\n");
                    else if (k == 1) fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                    else {
                        fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                        fprintf(out, "    imul rax, %d\n", k);
                    }
                } else if (in->b >= 0 && in->b < 65536 && has_known_const[in->b]) {
                    int k = known_const[in->b];
                    if (k == 0) fprintf(out, "    xor eax, eax\n");
                    else if (k == 1) fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                    else {
                        fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                        fprintf(out, "    imul rax, %d\n", k);
                    }
                } else {
                    fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                    fprintf(out, "    imul rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                }
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_DIV:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    cqo\n    idiv rbx\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_MOD:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rbx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    cqo\n    idiv rbx\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rdx\n", temp_offset(in->a));
                break;
            case IR_AND:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    and rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_OR:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    or rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_XOR:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    xor rax, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_SHL:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rcx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    shl rax, cl\n");
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
            case IR_SHR:
                fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", temp_offset(in->b));
                fprintf(out, "    mov rcx, QWORD PTR [rbp-%d]\n", temp_offset(in->c));
                fprintf(out, "    sar rax, cl\n");
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
                    cached_local_valid[0] = cached_local_valid[1] = false;
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
                            local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
                        }
                        if (local_cache_enabled) {
                            int cidx = -1;
                            for (int ci = 0; ci < 2; ++ci) {
                                if (cached_local_valid[ci] && strcmp(cached_local_name[ci], name) == 0) { cidx = ci; break; }
                            }
                            if (cidx < 0) {
                                for (int ci = 0; ci < 2; ++ci) {
                                    if (!cached_local_valid[ci]) { cidx = ci; break; }
                                }
                                if (cidx < 0) cidx = 0;
                                snprintf(cached_local_name[cidx], sizeof(cached_local_name[cidx]), "%.63s", name);
                                cached_local_valid[cidx] = true;
                            }
                            fprintf(out, "    mov %s, rax\n", cached_local_reg[cidx]);
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
                            local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
                        }
                        int cidx = -1;
                        if (local_cache_enabled) {
                            for (int ci = 0; ci < 2; ++ci) {
                                if (cached_local_valid[ci] && strcmp(cached_local_name[ci], name) == 0) { cidx = ci; break; }
                            }
                        }
                        if (cidx >= 0) {
                            fprintf(out, "    mov rax, %s\n", cached_local_reg[cidx]);
                        } else if (idx >= 0) {
                            fprintf(out, "    mov rax, QWORD PTR [rbp-%d]\n", local_slots[idx].offset);
                            if (local_cache_enabled) {
                                int free_idx = -1;
                                for (int ci = 0; ci < 2; ++ci) if (!cached_local_valid[ci]) { free_idx = ci; break; }
                                if (free_idx < 0) free_idx = 0;
                                snprintf(cached_local_name[free_idx], sizeof(cached_local_name[free_idx]), "%.63s", name);
                                cached_local_valid[free_idx] = true;
                                fprintf(out, "    mov %s, rax\n", cached_local_reg[free_idx]);
                            }
                        }
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
                        local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
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
                    local_cache_enabled = false;
                    cached_local_valid[0] = cached_local_valid[1] = false;
                    int idx = find_slot(local_slots, local_count, in->text);
                    if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                        idx = local_count++;
                        snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", in->text);
                        local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
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
                cached_local_valid[0] = cached_local_valid[1] = false;
                break;
            case IR_STRING:
                fprintf(out, "    lea rax, .LC%zu[rip]\n", i);
                fprintf(out, "    mov QWORD PTR [rbp-%d], rax\n", temp_offset(in->a));
                break;
        }
    }
}

static void emit_aarch64(IRProgram *ir, FILE *out) {
    int STACK_FRAME_SIZE = 0;
    int LOCALS_BASE_OFFSET = 0;
    compute_frame_layout(ir, &STACK_FRAME_SIZE, &LOCALS_BASE_OFFSET);

    fprintf(out, ".text\n");
    bool has_globals = false, has_strings = false;
    for (size_t i = 0; i < ir->count; ++i) {
        if (ir->insts[i].op == IR_GLOBAL) has_globals = true;
        if (ir->insts[i].op == IR_STRING) has_strings = true;
    }
    if (has_strings) {
        fprintf(out, ".section .rodata\n");
        for (size_t i = 0; i < ir->count; ++i) {
            IRInst *in = &ir->insts[i];
            if (in->op == IR_STRING) {
                fprintf(out, ".LC%zu:\n", i);
                fprintf(out, "    .asciz ");
                emit_escaped_c_string(out, in->text);
                fprintf(out, "\n");
            }
        }
    }
    if (has_globals) {
        fprintf(out, ".data\n");
        for (size_t i = 0; i < ir->count; ++i) {
            IRInst *in = &ir->insts[i];
            if (in->op == IR_GLOBAL) {
                fprintf(out, ".global %s\n%s:\n", in->text, in->text);
                fprintf(out, "    .xword %d\n", in->a);
            }
        }
    }
    fprintf(out, ".text\n");

    VarSlot local_slots[256];
    int local_count = 0;
    bool in_function = false;
    char cached_local_name[2][64] = {{0}};
    bool cached_local_valid[2] = {false, false};
    const char *cached_local_reg[2] = {"x9", "x10"};
    bool local_cache_enabled = true;
    int known_const[65536] = {0};
    unsigned char has_known_const[65536] = {0};

    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        if (in->op == IR_MOVI && in->a >= 0 && in->a < (int)(sizeof(known_const) / sizeof(known_const[0]))) {
            known_const[in->a] = in->b;
            has_known_const[in->a] = 1;
        }
    }

    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        switch (in->op) {
            case IR_LABEL:
                if (strncmp(in->text, "fn_", 3) == 0) {
                    local_count = 0;
                    in_function = true;
                    cached_local_valid[0] = cached_local_valid[1] = false;
                    local_cache_enabled = true;
                    fprintf(out, ".global %s\n%s:\n", in->text + 3, in->text + 3);
                    fprintf(out, "    stp x29, x30, [sp, #-16]!\n");
                    fprintf(out, "    mov x29, sp\n");
                    fprintf(out, "    sub sp, sp, #%d\n", STACK_FRAME_SIZE);
                } else fprintf(out, "%s:\n", in->text);
                break;
            case IR_NOP:
                break;
            case IR_MOVI:
                fprintf(out, "    mov x0, #%d\n", in->b);
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_ADD:
                if (in->b >= 0 && in->b < 65536 && has_known_const[in->b] &&
                    in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov x0, #%d\n", known_const[in->b] + known_const[in->c]);
                } else if (in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                    fprintf(out, "    mov x1, #%d\n", known_const[in->c]);
                    fprintf(out, "    add x0, x0, x1\n");
                } else if (in->b >= 0 && in->b < 65536 && has_known_const[in->b]) {
                    fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->c));
                    fprintf(out, "    mov x1, #%d\n", known_const[in->b]);
                    fprintf(out, "    add x0, x0, x1\n");
                } else {
                    fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                    fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                    fprintf(out, "    add x0, x0, x1\n");
                }
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_SUB:
                if (in->b >= 0 && in->b < 65536 && has_known_const[in->b] &&
                    in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov x0, #%d\n", known_const[in->b] - known_const[in->c]);
                } else if (in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                    fprintf(out, "    mov x1, #%d\n", known_const[in->c]);
                    fprintf(out, "    sub x0, x0, x1\n");
                } else if (in->b >= 0 && in->b < 65536 && has_known_const[in->b]) {
                    fprintf(out, "    mov x0, #%d\n", known_const[in->b]);
                    fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                    fprintf(out, "    sub x0, x0, x1\n");
                } else {
                    fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                    fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                    fprintf(out, "    sub x0, x0, x1\n");
                }
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_MUL:
                if (in->b >= 0 && in->b < 65536 && has_known_const[in->b] &&
                    in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    fprintf(out, "    mov x0, #%d\n", known_const[in->b] * known_const[in->c]);
                } else if (in->c >= 0 && in->c < 65536 && has_known_const[in->c]) {
                    int k = known_const[in->c];
                    if (k == 0) fprintf(out, "    mov x0, #0\n");
                    else if (k == 1) fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                    else {
                        fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                        fprintf(out, "    mov x1, #%d\n", k);
                        fprintf(out, "    mul x0, x0, x1\n");
                    }
                } else if (in->b >= 0 && in->b < 65536 && has_known_const[in->b]) {
                    int k = known_const[in->b];
                    if (k == 0) fprintf(out, "    mov x0, #0\n");
                    else if (k == 1) fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->c));
                    else {
                        fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->c));
                        fprintf(out, "    mov x1, #%d\n", k);
                        fprintf(out, "    mul x0, x0, x1\n");
                    }
                } else {
                    fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                    fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                    fprintf(out, "    mul x0, x0, x1\n");
                }
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_DIV:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                fprintf(out, "    sdiv x0, x0, x1\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_MOD:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                fprintf(out, "    sdiv x2, x0, x1\n");
                fprintf(out, "    msub x0, x2, x1, x0\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_AND:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                fprintf(out, "    and x0, x0, x1\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_OR:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                fprintf(out, "    orr x0, x0, x1\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_XOR:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                fprintf(out, "    eor x0, x0, x1\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_SHL:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                fprintf(out, "    lsl x0, x0, x1\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_SHR:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->c));
                fprintf(out, "    asr x0, x0, x1\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_CMP:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->a));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    cmp x0, x1\n");
                if (strcmp(in->text, "==") == 0) fprintf(out, "    cset x0, eq\n");
                else if (strcmp(in->text, "!=") == 0) fprintf(out, "    cset x0, ne\n");
                else if (strcmp(in->text, "<") == 0) fprintf(out, "    cset x0, lt\n");
                else if (strcmp(in->text, "<=") == 0) fprintf(out, "    cset x0, le\n");
                else if (strcmp(in->text, ">") == 0) fprintf(out, "    cset x0, gt\n");
                else if (strcmp(in->text, ">=") == 0) fprintf(out, "    cset x0, ge\n");
                else if (strcmp(in->text, "&&") == 0) { fprintf(out, "    cset x0, ne\n"); }
                else if (strcmp(in->text, "||") == 0) { fprintf(out, "    cset x0, ne\n"); }
                else fprintf(out, "    cset x0, ne\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->c));
                break;
            case IR_JMP:
                fprintf(out, "    b L%d\n", in->a);
                break;
            case IR_JE:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->a));
                fprintf(out, "    cmp x0, #0\n");
                fprintf(out, "    beq L%d\n", in->b);
                break;
            case IR_JNE:
                fprintf(out, "    bne L%d\n", in->b);
                break;
            case IR_CALL: {
                int args[16] = {0};
                char callee[64] = {0};
                int argc = parse_call_descriptor(in->text, callee, sizeof(callee), args, 16);
                int stack_args = (argc > 8) ? (argc - 8) : 0;
                if (stack_args) fprintf(out, "    sub sp, sp, #%d\n", stack_args * 8);
                for (int ai = argc - 1; ai >= 8; --ai) {
                    int ofs = (ai - 8) * 8;
                    fprintf(out, "    ldr x9, [sp, #%d]\n", temp_offset(args[ai]) + stack_args * 8);
                    fprintf(out, "    str x9, [sp, #%d]\n", ofs);
                }
                for (int ai = 0; ai < argc && ai < 8; ++ai) {
                    fprintf(out, "    ldr x%d, [sp, #%d]\n", ai, temp_offset(args[ai]) + stack_args * 8);
                }
                fprintf(out, "    bl %s\n", callee[0] ? callee : in->text);
                if (stack_args) fprintf(out, "    add sp, sp, #%d\n", stack_args * 8);
                cached_local_valid[0] = cached_local_valid[1] = false;
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            }
            case IR_STORE: {
                char name[64] = {0};
                if (!parse_store_text(in->text, name, sizeof(name))) break;
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->a));
                if (is_global_symbol(ir, name)) {
                    fprintf(out, "    adrp x10, %s\n", name);
                    fprintf(out, "    add x10, x10, :lo12:%s\n", name);
                    fprintf(out, "    str x0, [x10]\n");
                } else {
                    int idx = find_slot(local_slots, local_count, name);
                    if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                        idx = local_count++;
                        snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", name);
                        local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
                    }
                    if (local_cache_enabled) {
                        int cidx = -1;
                        for (int ci = 0; ci < 2; ++ci) {
                            if (cached_local_valid[ci] && strcmp(cached_local_name[ci], name) == 0) { cidx = ci; break; }
                        }
                        if (cidx < 0) {
                            for (int ci = 0; ci < 2; ++ci) {
                                if (!cached_local_valid[ci]) { cidx = ci; break; }
                            }
                            if (cidx < 0) cidx = 0;
                            snprintf(cached_local_name[cidx], sizeof(cached_local_name[cidx]), "%.63s", name);
                            cached_local_valid[cidx] = true;
                        }
                        fprintf(out, "    mov %s, x0\n", cached_local_reg[cidx]);
                    }
                    if (idx >= 0) fprintf(out, "    str x0, [sp, #%d]\n", local_slots[idx].offset);
                }
                break;
            }
            case IR_LOAD: {
                char name[64] = {0};
                if (!parse_load_text(in->text, name, sizeof(name))) break;
                if (is_global_symbol(ir, name)) {
                    fprintf(out, "    adrp x10, %s\n", name);
                    fprintf(out, "    add x10, x10, :lo12:%s\n", name);
                    fprintf(out, "    ldr x0, [x10]\n");
                } else {
                    int idx = find_slot(local_slots, local_count, name);
                    if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                        idx = local_count++;
                        snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", name);
                        local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
                    }
                    int cidx = -1;
                    if (local_cache_enabled) {
                        for (int ci = 0; ci < 2; ++ci) {
                            if (cached_local_valid[ci] && strcmp(cached_local_name[ci], name) == 0) { cidx = ci; break; }
                        }
                    }
                    if (cidx >= 0) {
                        fprintf(out, "    mov x0, %s\n", cached_local_reg[cidx]);
                    } else if (idx >= 0) {
                        fprintf(out, "    ldr x0, [sp, #%d]\n", local_slots[idx].offset);
                        if (local_cache_enabled) {
                            int free_idx = -1;
                            for (int ci = 0; ci < 2; ++ci) if (!cached_local_valid[ci]) { free_idx = ci; break; }
                            if (free_idx < 0) free_idx = 0;
                            snprintf(cached_local_name[free_idx], sizeof(cached_local_name[free_idx]), "%.63s", name);
                            cached_local_valid[free_idx] = true;
                            fprintf(out, "    mov %s, x0\n", cached_local_reg[free_idx]);
                        }
                    }
                }
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            }
            case IR_RET:
                if (in->a >= 0) fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->a));
                if (in_function) {
                    fprintf(out, "    add sp, sp, #%d\n", STACK_FRAME_SIZE);
                    fprintf(out, "    ldp x29, x30, [sp], #16\n");
                }
                fprintf(out, "    ret\n");
                break;
            case IR_GLOBAL:
                break;
            case IR_PARAM: {
                int idx = find_slot(local_slots, local_count, in->text);
                if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                    idx = local_count++;
                    snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", in->text);
                    local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
                }
                if (idx >= 0) {
                    if (in->a < 8) fprintf(out, "    str x%d, [sp, #%d]\n", in->a, local_slots[idx].offset);
                    else {
                        int stack_off = (in->a - 8) * 8;
                        fprintf(out, "    ldr x0, [x29, #%d]\n", 16 + stack_off);
                        fprintf(out, "    str x0, [sp, #%d]\n", local_slots[idx].offset);
                    }
                }
                break;
            }
            case IR_ADDR:
                if (is_global_symbol(ir, in->text)) {
                    fprintf(out, "    adrp x0, %s\n", in->text);
                    fprintf(out, "    add x0, x0, :lo12:%s\n", in->text);
                } else {
                    local_cache_enabled = false;
                    cached_local_valid[0] = cached_local_valid[1] = false;
                    int idx = find_slot(local_slots, local_count, in->text);
                    if (idx < 0 && local_count < (int)(sizeof(local_slots) / sizeof(local_slots[0]))) {
                        idx = local_count++;
                        snprintf(local_slots[idx].name, sizeof(local_slots[idx].name), "%.63s", in->text);
                        local_slots[idx].offset = LOCALS_BASE_OFFSET + idx * 8;
                    }
                    if (idx >= 0) fprintf(out, "    add x0, sp, #%d\n", local_slots[idx].offset);
                }
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_LOAD_IND:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    ldr x0, [x0]\n");
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
            case IR_STORE_IND:
                fprintf(out, "    ldr x0, [sp, #%d]\n", temp_offset(in->a));
                fprintf(out, "    ldr x1, [sp, #%d]\n", temp_offset(in->b));
                fprintf(out, "    str x1, [x0]\n");
                cached_local_valid[0] = cached_local_valid[1] = false;
                break;
            case IR_STRING:
                fprintf(out, "    adrp x0, .LC%zu\n", i);
                fprintf(out, "    add x0, x0, :lo12:.LC%zu\n", i);
                fprintf(out, "    str x0, [sp, #%d]\n", temp_offset(in->a));
                break;
        }
    }
}

static bool target_is_x86_64(const char *target) {
    if (!target) return true;
    return strcmp(target, "x86-64") == 0 || strcmp(target, "x86_64") == 0 || strcmp(target, "amd64") == 0;
}

bool emit_target(IRProgram *ir, FILE *out, const char *target) {
    if (target_is_x86_64(target)) {
        emit_x86_64(ir, out);
        return true;
    }
    if (target && (strcmp(target, "arm-64") == 0 || strcmp(target, "aarch64") == 0 || strcmp(target, "arm64") == 0)) {
        emit_aarch64(ir, out);
        return true;
    }
    fprintf(stderr, "GEE backend for target '%s' is not implemented yet.\n", target ? target : "(null)");
    return false;
}
