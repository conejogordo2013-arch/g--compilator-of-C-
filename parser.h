#ifndef GEE_PARSER_H
#define GEE_PARSER_H

#include "ast.h"
#include "lexer.h"
#include <stdbool.h>

typedef struct Symbol {
    char *name;
    TypeKind type;
    bool is_function;
    TypeKind return_type;
    size_t arity;
    TypeKind params[16];
    bool variadic;
    char *struct_name;
    bool is_extern;
    bool is_imported;
    bool referenced;
} Symbol;

typedef struct SymbolTable {
    Symbol *symbols;
    size_t count;
    size_t capacity;
    struct SymbolTable *parent;
} SymbolTable;

typedef struct Parser {
    TokenBuffer tokens;
    size_t current;
    bool had_error;
    SymbolTable *scope;
    TypeKind current_function_return;
    int loop_depth;
    struct {
        char name[64];
        struct {
            char name[64];
            TypeKind type;
            int offset;
        } fields[32];
        int field_count;
        int size;
    } structs[64];
    int struct_count;
    char pending_struct_name[64];
} Parser;

AstNode *parse_program(Parser *p, const char *src);
void parser_destroy(Parser *p);

#endif
