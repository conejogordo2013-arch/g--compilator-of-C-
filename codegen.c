#include "codegen.h"

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
            for (size_t i = 0; i < n->as.call_expr.args.count; ++i) {
                (void)gen_expr(n->as.call_expr.args.items[i], ir);
            }
            int out = fresh_temp(ir);
            ir_emit(ir, IR_CALL, out, (int)n->as.call_expr.args.count, 0, n->as.call_expr.name);
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
            gen_stmt(d->as.function.body, ir);
            ir_emit(ir, IR_NOP, 0, 0, 0, "epilogue: mov rsp, rbp; pop rbp");
            ir_emit(ir, IR_RET, -1, 0, 0, "ret");
        }
    }
    current_program = NULL;
}

void emit_x86_64(IRProgram *ir, FILE *out) {
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

    for (size_t i = 0; i < ir->count; ++i) {
        IRInst *in = &ir->insts[i];
        switch (in->op) {
            case IR_LABEL:
                if (strncmp(in->text, "fn_", 3) == 0) fprintf(out, ".globl %s\n%s:\n", in->text + 3, in->text + 3);
                else fprintf(out, "%s:\n", in->text);
                break;
            case IR_NOP:
                fprintf(out, "    # %s\n", in->text);
                break;
            case IR_MOVI:
                fprintf(out, "    # t%d = %d\n", in->a, in->b);
                fprintf(out, "    mov rax, %d\n", in->b);
                break;
            case IR_ADD:
                fprintf(out, "    # t%d = t%d + t%d\n", in->a, in->b, in->c);
                fprintf(out, "    add rax, rbx\n");
                break;
            case IR_SUB:
                fprintf(out, "    # t%d = t%d - t%d\n", in->a, in->b, in->c);
                fprintf(out, "    sub rax, rbx\n");
                break;
            case IR_MUL:
                fprintf(out, "    # t%d = t%d * t%d\n", in->a, in->b, in->c);
                fprintf(out, "    imul rax, rbx\n");
                break;
            case IR_DIV:
                fprintf(out, "    # t%d = t%d / t%d\n", in->a, in->b, in->c);
                fprintf(out, "    cqo\n    idiv rbx\n");
                break;
            case IR_CMP:
                fprintf(out, "    # cmp %s\n", in->text);
                fprintf(out, "    cmp rax, rbx\n");
                break;
            case IR_JMP:
                fprintf(out, "    jmp L%d\n", in->a);
                break;
            case IR_JE:
                fprintf(out, "    cmp rax, 0\n");
                fprintf(out, "    je L%d\n", in->b);
                break;
            case IR_JNE:
                fprintf(out, "    jne L%d\n", in->b);
                break;
            case IR_CALL:
                fprintf(out, "    # call %s with %d args\n", in->text, in->b);
                fprintf(out, "    call %s\n", in->text);
                break;
            case IR_STORE:
                fprintf(out, "    # %s\n", in->text);
                fprintf(out, "    # TODO: map variable to stack slot and store from temp register\n");
                break;
            case IR_LOAD:
                fprintf(out, "    # %s\n", in->text);
                fprintf(out, "    # TODO: map variable stack slot into temp register\n");
                break;
            case IR_RET:
                fprintf(out, "    ret\n");
                break;
            case IR_GLOBAL:
                /* Emitted in data prelude above. */
                break;
        }
    }
}
