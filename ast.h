#ifndef GEE_AST_H
#define GEE_AST_H

#include <stdbool.h>
#include <stddef.h>

typedef enum TypeKind {
    TYPE_UNKNOWN = 0,
    TYPE_VOID,
    TYPE_BYTE,
    TYPE_INT32,
    TYPE_INT64,
    TYPE_STRING,
    TYPE_BOOL
} TypeKind;

typedef enum AstKind {
    AST_PROGRAM = 0,
    AST_IMPORT,
    AST_EXTERN_DECL,
    AST_FUNCTION,
    AST_PARAM,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_SWITCH,
    AST_CASE,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_EXPR_STMT,
    AST_CALL,
    AST_BINARY,
    AST_UNARY,
    AST_ASSIGN,
    AST_IDENTIFIER,
    AST_INT_LITERAL,
    AST_STRING_LITERAL
} AstKind;

typedef struct AstNode AstNode;

typedef struct NodeArray {
    AstNode **items;
    size_t count;
    size_t capacity;
} NodeArray;

struct AstNode {
    AstKind kind;
    TypeKind type;
    int line;

    union {
        struct {
            NodeArray decls;
        } program;

        struct {
            char *module;
        } import_decl;

        struct {
            char *name;
            TypeKind return_type;
            NodeArray params;
        } extern_decl;

        struct {
            char *name;
            TypeKind return_type;
            NodeArray params;
            AstNode *body;
            bool can_inline;
            bool dead;
        } function;

        struct {
            char *name;
            TypeKind param_type;
        } param;

        struct {
            NodeArray statements;
        } block;

        struct {
            char *name;
            TypeKind decl_type;
            AstNode *init;
        } var_decl;

        struct {
            AstNode *cond;
            AstNode *then_branch;
            AstNode *else_branch;
        } if_stmt;

        struct {
            AstNode *cond;
            AstNode *body;
        } while_stmt;

        struct {
            AstNode *init;
            AstNode *cond;
            AstNode *post;
            AstNode *body;
        } for_stmt;

        struct {
            AstNode *expr;
            NodeArray cases;
            AstNode *default_case;
        } switch_stmt;

        struct {
            AstNode *value;
            AstNode *body;
        } case_stmt;

        struct {
            AstNode *expr;
        } return_stmt;

        struct {
            AstNode *expr;
        } expr_stmt;

        struct {
            char *name;
            NodeArray args;
        } call_expr;

        struct {
            char op[3];
            AstNode *lhs;
            AstNode *rhs;
        } binary_expr;

        struct {
            char op[3];
            AstNode *expr;
        } unary_expr;

        struct {
            char *target;
            AstNode *value;
        } assign_expr;

        struct {
            char *name;
        } ident_expr;

        struct {
            long long value;
        } int_lit;

        struct {
            char *value;
        } str_lit;
    } as;
};

void node_array_push(NodeArray *arr, AstNode *node);
AstNode *ast_new(AstKind kind, int line);
void ast_free(AstNode *node);
const char *type_kind_name(TypeKind t);

#endif
