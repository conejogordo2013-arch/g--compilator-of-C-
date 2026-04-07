#ifndef GEE_CODEGEN_H
#define GEE_CODEGEN_H

#include "ast.h"
#include <stdio.h>

typedef enum IROp {
    IR_NOP = 0,
    IR_LABEL,
    IR_MOVI,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_AND,
    IR_OR,
    IR_XOR,
    IR_SHL,
    IR_SHR,
    IR_CMP,
    IR_JMP,
    IR_JE,
    IR_JNE,
    IR_CALL,
    IR_RET,
    IR_STORE,
    IR_LOAD,
    IR_GLOBAL,
    IR_PARAM,
    IR_ADDR,
    IR_LOAD_IND,
    IR_STORE_IND,
    IR_STRING
} IROp;

typedef struct IRInst {
    IROp op;
    int a;
    int b;
    int c;
    int line;
    char text[96];
} IRInst;

typedef struct IRProgram {
    IRInst *insts;
    size_t count;
    size_t capacity;
    int temp_counter;
    int label_counter;
} IRProgram;

void ir_init(IRProgram *ir);
void ir_free(IRProgram *ir);
void generate_ir(AstNode *program, IRProgram *ir);
void optimize_ir(IRProgram *ir);
void emit_x86_64(IRProgram *ir, FILE *out);
bool emit_target(IRProgram *ir, FILE *out, const char *target);

#endif
