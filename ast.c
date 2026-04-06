#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void node_array_push(NodeArray *arr, AstNode *node) {
    if (arr->count == arr->capacity) {
        arr->capacity = arr->capacity ? arr->capacity * 2 : 16;
        arr->items = (AstNode **)realloc(arr->items, arr->capacity * sizeof(AstNode *));
        if (!arr->items) {
            fprintf(stderr, "fatal: out of memory in ast array\n");
            exit(1);
        }
    }
    arr->items[arr->count++] = node;
}

AstNode *ast_new(AstKind kind, int line) {
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    if (!n) {
        fprintf(stderr, "fatal: out of memory allocating ast node\n");
        exit(1);
    }
    n->kind = kind;
    n->type = TYPE_UNKNOWN;
    n->line = line;
    return n;
}

static void free_array(NodeArray *arr) {
    for (size_t i = 0; i < arr->count; ++i) ast_free(arr->items[i]);
    free(arr->items);
}

void ast_free(AstNode *node) {
    if (!node) return;

    switch (node->kind) {
        case AST_PROGRAM:
            free_array(&node->as.program.decls);
            break;
        case AST_IMPORT:
            free(node->as.import_decl.module);
            break;
        case AST_EXTERN_DECL:
            free(node->as.extern_decl.name);
            free_array(&node->as.extern_decl.params);
            break;
        case AST_FUNCTION:
            free(node->as.function.name);
            free_array(&node->as.function.params);
            ast_free(node->as.function.body);
            break;
        case AST_PARAM:
            free(node->as.param.name);
            break;
        case AST_BLOCK:
            free_array(&node->as.block.statements);
            break;
        case AST_VAR_DECL:
            free(node->as.var_decl.name);
            ast_free(node->as.var_decl.init);
            break;
        case AST_IF:
            ast_free(node->as.if_stmt.cond);
            ast_free(node->as.if_stmt.then_branch);
            ast_free(node->as.if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->as.while_stmt.cond);
            ast_free(node->as.while_stmt.body);
            break;
        case AST_FOR:
            ast_free(node->as.for_stmt.init);
            ast_free(node->as.for_stmt.cond);
            ast_free(node->as.for_stmt.post);
            ast_free(node->as.for_stmt.body);
            break;
        case AST_SWITCH:
            ast_free(node->as.switch_stmt.expr);
            free_array(&node->as.switch_stmt.cases);
            ast_free(node->as.switch_stmt.default_case);
            break;
        case AST_CASE:
            ast_free(node->as.case_stmt.value);
            ast_free(node->as.case_stmt.body);
            break;
        case AST_RETURN:
            ast_free(node->as.return_stmt.expr);
            break;
        case AST_EXPR_STMT:
            ast_free(node->as.expr_stmt.expr);
            break;
        case AST_CALL:
            free(node->as.call_expr.name);
            free_array(&node->as.call_expr.args);
            break;
        case AST_BINARY:
            ast_free(node->as.binary_expr.lhs);
            ast_free(node->as.binary_expr.rhs);
            break;
        case AST_UNARY:
            ast_free(node->as.unary_expr.expr);
            break;
        case AST_ASSIGN:
            free(node->as.assign_expr.target);
            ast_free(node->as.assign_expr.value);
            break;
        case AST_PTR_ASSIGN:
            ast_free(node->as.ptr_assign_expr.ptr);
            ast_free(node->as.ptr_assign_expr.value);
            break;
        case AST_FIELD:
            ast_free(node->as.field_expr.base);
            break;
        case AST_FIELD_ASSIGN:
            ast_free(node->as.field_assign_expr.base);
            ast_free(node->as.field_assign_expr.value);
            break;
        case AST_IDENTIFIER:
            free(node->as.ident_expr.name);
            break;
        case AST_STRING_LITERAL:
            free(node->as.str_lit.value);
            break;
        default:
            break;
    }

    free(node);
}

const char *type_kind_name(TypeKind t) {
    switch (t) {
        case TYPE_VOID: return "void";
        case TYPE_BYTE: return "byte";
        case TYPE_INT32: return "int32";
        case TYPE_INT64: return "int64";
        case TYPE_STRING: return "string";
        case TYPE_BOOL: return "bool";
        case TYPE_PTR: return "ptr";
        case TYPE_STRUCT: return "struct";
        default: return "unknown";
    }
}
