#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    memcpy(p, s, n + 1);
    return p;
}

static char *dup_lexeme(Token t) {
    char *s = (char *)malloc(t.length + 1);
    memcpy(s, t.lexeme, t.length);
    s[t.length] = '\0';
    return s;
}

static void parse_error(Parser *p, Token t, const char *msg) {
    fprintf(stderr, "parse error at %d:%d near '%.*s': %s\n", t.line, t.col, (int)t.length, t.lexeme, msg);
    p->had_error = true;
}

static Token peek(Parser *p) { return p->tokens.data[p->current]; }
static Token prev(Parser *p) { return p->tokens.data[p->current - 1]; }
static bool at_end(Parser *p) { return peek(p).kind == TOK_EOF; }
static Token peek_n(Parser *p, size_t n) {
    size_t idx = p->current + n;
    if (idx >= p->tokens.count) return p->tokens.data[p->tokens.count - 1];
    return p->tokens.data[idx];
}

static bool match_symbol(Parser *p, char c) {
    Token t = peek(p);
    if (t.kind == TOK_SYMBOL && t.length == 1 && t.lexeme[0] == c) { p->current++; return true; }
    return false;
}

static bool match_op(Parser *p, const char *op) {
    Token t = peek(p);
    size_t n = strlen(op);
    if (t.kind == TOK_OPERATOR && t.length == n && strncmp(t.lexeme, op, n) == 0) {
        p->current++;
        return true;
    }
    return false;
}

static bool match_kw(Parser *p, Keyword kw) {
    Token t = peek(p);
    if (t.kind == TOK_KEYWORD && t.keyword == kw) { p->current++; return true; }
    return false;
}

static bool match_ellipsis(Parser *p) {
    Token a = peek_n(p, 0), b = peek_n(p, 1), c = peek_n(p, 2);
    if (a.kind == TOK_SYMBOL && b.kind == TOK_SYMBOL && c.kind == TOK_SYMBOL &&
        a.length == 1 && b.length == 1 && c.length == 1 &&
        a.lexeme[0] == '.' && b.lexeme[0] == '.' && c.lexeme[0] == '.') {
        p->current += 3;
        return true;
    }
    return false;
}

static Token consume(Parser *p, TokenKind kind, const char *msg) {
    Token t = peek(p);
    if (t.kind == kind) { p->current++; return t; }
    parse_error(p, t, msg);
    return t;
}

static SymbolTable *scope_push(SymbolTable *parent) {
    SymbolTable *s = (SymbolTable *)calloc(1, sizeof(SymbolTable));
    s->parent = parent;
    return s;
}

static void scope_free(SymbolTable *s) {
    for (size_t i = 0; i < s->count; ++i) {
        free(s->symbols[i].name);
        free(s->symbols[i].struct_name);
    }
    free(s->symbols);
    free(s);
}

static Symbol *sym_lookup(SymbolTable *s, const char *name) {
    for (SymbolTable *it = s; it; it = it->parent) {
        for (size_t i = 0; i < it->count; ++i) {
            if (strcmp(it->symbols[i].name, name) == 0) return &it->symbols[i];
        }
    }
    return NULL;
}

static Symbol *sym_insert(SymbolTable *s, Symbol sym) {
    if (s->count == s->capacity) {
        s->capacity = s->capacity ? s->capacity * 2 : 16;
        s->symbols = (Symbol *)realloc(s->symbols, s->capacity * sizeof(Symbol));
    }
    s->symbols[s->count] = sym;
    return &s->symbols[s->count++];
}

static TypeKind parse_type(Parser *p) {
    p->pending_struct_name[0] = '\0';
    TypeKind base = TYPE_UNKNOWN;
    if (match_kw(p, KW_INT32)) base = TYPE_INT32;
    else if (match_kw(p, KW_INT64)) base = TYPE_INT64;
    else if (match_kw(p, KW_BYTE)) base = TYPE_BYTE;
    else if (match_kw(p, KW_STRUCT)) {
        Token st = consume(p, TOK_IDENTIFIER, "expected struct name");
        size_t n = st.length < sizeof(p->pending_struct_name) - 1 ? st.length : sizeof(p->pending_struct_name) - 1;
        memcpy(p->pending_struct_name, st.lexeme, n);
        p->pending_struct_name[n] = '\0';
        base = TYPE_STRUCT;
    }
    else if (match_kw(p, KW_BOOL)) base = TYPE_BOOL;
    else if (match_kw(p, KW_STRING)) base = TYPE_STRING;
    else if (match_kw(p, KW_VOID)) base = TYPE_VOID;
    if (base == TYPE_UNKNOWN) {
        parse_error(p, peek(p), "expected type keyword");
        return TYPE_UNKNOWN;
    }

    while (match_op(p, "*")) {
        base = TYPE_PTR;
    }

    return base;
}

static bool numeric(TypeKind t) {
    return t == TYPE_INT32 || t == TYPE_INT64 || t == TYPE_BYTE;
}

static int type_size(TypeKind t) {
    switch (t) {
        case TYPE_BYTE: return 1;
        case TYPE_INT32: return 4;
        case TYPE_INT64: return 8;
        case TYPE_PTR: return 8;
        case TYPE_BOOL: return 1;
        default: return 8;
    }
}

static int align_to(int x, int a) {
    return (x + (a - 1)) & ~(a - 1);
}

static int find_struct(Parser *p, const char *name) {
    for (int i = 0; i < p->struct_count; ++i) {
        if (strcmp(p->structs[i].name, name) == 0) return i;
    }
    return -1;
}

static TypeKind unify_numeric(TypeKind a, TypeKind b) {
    if (!numeric(a) || !numeric(b)) return TYPE_UNKNOWN;
    if (a == TYPE_INT64 || b == TYPE_INT64) return TYPE_INT64;
    if (a == TYPE_INT32 || b == TYPE_INT32) return TYPE_INT32;
    return TYPE_BYTE;
}

static AstNode *parse_expr(Parser *p);
static AstNode *parse_statement(Parser *p);
static AstNode *parse_block(Parser *p);

static AstNode *parse_primary(Parser *p) {
    Token t = peek(p);
    if (t.kind == TOK_KEYWORD && (t.keyword == KW_TRUE || t.keyword == KW_FALSE)) {
        p->current++;
        AstNode *n = ast_new(AST_INT_LITERAL, t.line);
        n->type = TYPE_BOOL;
        n->as.int_lit.value = (t.keyword == KW_TRUE) ? 1 : 0;
        return n;
    }
    if (t.kind == TOK_INTEGER) {
        p->current++;
        AstNode *n = ast_new(AST_INT_LITERAL, t.line);
        n->type = TYPE_INT64;
        n->as.int_lit.value = t.int_value;
        return n;
    }
    if (t.kind == TOK_STRING) {
        p->current++;
        AstNode *n = ast_new(AST_STRING_LITERAL, t.line);
        n->type = TYPE_STRING;
        n->as.str_lit.value = xstrdup(t.str_value);
        return n;
    }
    if (t.kind == TOK_IDENTIFIER) {
        p->current++;
        char *name = dup_lexeme(t);

        if (match_symbol(p, '(')) {
            AstNode *call = ast_new(AST_CALL, t.line);
            call->as.call_expr.name = name;
            while (!match_symbol(p, ')') && !at_end(p)) {
                AstNode *arg = parse_expr(p);
                node_array_push(&call->as.call_expr.args, arg);
                if (!match_symbol(p, ',')) {
                    if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' in call");
                    break;
                }
            }

            Symbol *fn = sym_lookup(p->scope, call->as.call_expr.name);
            if (fn && fn->is_function) {
                fn->referenced = true;
                call->type = fn->return_type;
                if ((!fn->variadic && fn->arity != call->as.call_expr.args.count) ||
                    (fn->variadic && call->as.call_expr.args.count < fn->arity)) {
                    parse_error(p, t, "argument count mismatch in call");
                }
            } else {
                parse_error(p, t, "unknown function called");
                call->type = TYPE_UNKNOWN;
            }
            return call;
        }

        AstNode *id = ast_new(AST_IDENTIFIER, t.line);
        id->as.ident_expr.name = name;
        Symbol *sym = sym_lookup(p->scope, name);
        if (sym) {
            sym->referenced = true;
            id->type = sym->type;
        } else {
            parse_error(p, t, "use of undeclared identifier");
            id->type = TYPE_UNKNOWN;
        }
        AstNode *expr = id;
        Symbol *base_sym = sym;
        while (match_symbol(p, '.') || match_op(p, "->")) {
            bool through_ptr = prev(p).kind == TOK_OPERATOR;
            Token fld = consume(p, TOK_IDENTIFIER, "expected field name");
            if (!base_sym || !base_sym->struct_name) {
                parse_error(p, fld, "field access requires struct-typed symbol");
                break;
            }
            int si = find_struct(p, base_sym->struct_name);
            if (si < 0) {
                parse_error(p, fld, "unknown struct in field access");
                break;
            }
            int off = -1;
            TypeKind fty = TYPE_UNKNOWN;
            for (int fi = 0; fi < p->structs[si].field_count; ++fi) {
                if (strlen(p->structs[si].fields[fi].name) == fld.length &&
                    strncmp(p->structs[si].fields[fi].name, fld.lexeme, fld.length) == 0) {
                    off = p->structs[si].fields[fi].offset;
                    fty = p->structs[si].fields[fi].type;
                    break;
                }
            }
            if (off < 0) {
                parse_error(p, fld, "unknown struct field");
                break;
            }
            AstNode *fn = ast_new(AST_FIELD, fld.line);
            fn->as.field_expr.base = expr;
            fn->as.field_expr.offset = off;
            fn->as.field_expr.through_ptr = through_ptr;
            fn->type = fty;
            expr = fn;
            base_sym = NULL;
        }
        return expr;
    }

    if (match_symbol(p, '(')) {
        AstNode *e = parse_expr(p);
        if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after expression");
        return e;
    }

    parse_error(p, t, "expected primary expression");
    p->current++;
    return ast_new(AST_INT_LITERAL, t.line);
}

static AstNode *parse_unary(Parser *p) {
    if (match_op(p, "++") || match_op(p, "--")) {
        Token op = prev(p);
        AstNode *target = parse_unary(p);
        if (target->kind != AST_IDENTIFIER) {
            parse_error(p, op, "prefix ++/-- expects identifier target");
            return target;
        }

        AstNode *lhs_val = ast_new(AST_IDENTIFIER, op.line);
        lhs_val->as.ident_expr.name = xstrdup(target->as.ident_expr.name);
        lhs_val->type = target->type;

        AstNode *one = ast_new(AST_INT_LITERAL, op.line);
        one->type = TYPE_INT64;
        one->as.int_lit.value = 1;

        AstNode *bin = ast_new(AST_BINARY, op.line);
        snprintf(bin->as.binary_expr.op, sizeof(bin->as.binary_expr.op), "%c", op.lexeme[0]);
        bin->as.binary_expr.lhs = lhs_val;
        bin->as.binary_expr.rhs = one;

        bin->type = unify_numeric(lhs_val->type, one->type);
        if (bin->type == TYPE_UNKNOWN && lhs_val->type == TYPE_PTR) {
            bin->type = TYPE_PTR;
        }
        if (bin->type == TYPE_UNKNOWN) {
            parse_error(p, op, "invalid operand type for ++/--");
        }

        AstNode *assign = ast_new(AST_ASSIGN, op.line);
        assign->as.assign_expr.target = xstrdup(target->as.ident_expr.name);
        assign->as.assign_expr.value = bin;
        assign->type = bin->type;

        ast_free(target);
        return assign;
    }

    if (match_op(p, "-") || match_op(p, "!") || match_op(p, "~") || match_op(p, "&") || match_op(p, "*")) {
        Token op = prev(p);
        AstNode *rhs = parse_unary(p);
        AstNode *n = ast_new(AST_UNARY, op.line);
        snprintf(n->as.unary_expr.op, sizeof(n->as.unary_expr.op), "%.*s", (int)op.length, op.lexeme);
        n->as.unary_expr.expr = rhs;
        if (op.length == 1 && op.lexeme[0] == '&') n->type = TYPE_PTR;
        else if (op.length == 1 && op.lexeme[0] == '*') n->type = TYPE_INT64;
        else if (op.length == 1 && op.lexeme[0] == '~') {
            n->type = rhs->type;
            if (!numeric(rhs->type)) parse_error(p, op, "invalid type for bitwise not");
        } else n->type = rhs->type;
        return n;
    }
    return parse_primary(p);
}

static AstNode *fold_binary(const char *op, AstNode *lhs, AstNode *rhs, int line) {
    if (lhs->kind == AST_INT_LITERAL && rhs->kind == AST_INT_LITERAL) {
        long long a = lhs->as.int_lit.value;
        long long b = rhs->as.int_lit.value;
        AstNode *n = ast_new(AST_INT_LITERAL, line);
        if (strcmp(op, "+") == 0) n->as.int_lit.value = a + b;
        else if (strcmp(op, "-") == 0) n->as.int_lit.value = a - b;
        else if (strcmp(op, "*") == 0) n->as.int_lit.value = a * b;
        else if (strcmp(op, "/") == 0 && b != 0) n->as.int_lit.value = a / b;
        else if (strcmp(op, "%") == 0 && b != 0) n->as.int_lit.value = a % b;
        else if (strcmp(op, "&") == 0) n->as.int_lit.value = a & b;
        else if (strcmp(op, "|") == 0) n->as.int_lit.value = a | b;
        else if (strcmp(op, "^") == 0) n->as.int_lit.value = a ^ b;
        else if (strcmp(op, "<<") == 0) n->as.int_lit.value = a << b;
        else if (strcmp(op, ">>") == 0) n->as.int_lit.value = a >> b;
        else { ast_free(n); return NULL; }
        n->type = unify_numeric(lhs->type, rhs->type);
        return n;
    }
    return NULL;
}

static AstNode *parse_mul(Parser *p) {
    AstNode *lhs = parse_unary(p);
    while (match_op(p, "*") || match_op(p, "/") || match_op(p, "%")) {
        Token op = prev(p);
        AstNode *rhs = parse_unary(p);
        char opbuf[3] = {0};
        snprintf(opbuf, sizeof(opbuf), "%.*s", (int)op.length, op.lexeme);
        AstNode *folded = fold_binary(opbuf, lhs, rhs, op.line);
        if (folded) {
            lhs = folded;
            continue;
        }
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "%.*s", (int)op.length, op.lexeme);
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        n->type = unify_numeric(lhs->type, rhs->type);
        if (n->type == TYPE_UNKNOWN) parse_error(p, op, "invalid types for multiplicative operator");
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_add(Parser *p) {
    AstNode *lhs = parse_mul(p);
    while (match_op(p, "+") || match_op(p, "-")) {
        Token op = prev(p);
        AstNode *rhs = parse_mul(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "%.*s", (int)op.length, op.lexeme);
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        AstNode *folded = fold_binary(n->as.binary_expr.op, lhs, rhs, op.line);
        if (folded) {
            ast_free(n);
            lhs = folded;
            continue;
        }
        n->type = unify_numeric(lhs->type, rhs->type);
        if (n->type == TYPE_UNKNOWN) {
            bool lhs_ptr = (lhs->type == TYPE_PTR);
            bool rhs_ptr = (rhs->type == TYPE_PTR);
            bool lhs_num = numeric(lhs->type);
            bool rhs_num = numeric(rhs->type);
            if (op.length == 1 && op.lexeme[0] == '+') {
                if ((lhs_ptr && rhs_num) || (rhs_ptr && lhs_num)) {
                    n->type = TYPE_PTR;
                }
            } else if (op.length == 1 && op.lexeme[0] == '-') {
                if (lhs_ptr && rhs_num) {
                    n->type = TYPE_PTR;
                }
            }
        }
        if (n->type == TYPE_UNKNOWN) parse_error(p, op, "invalid types for additive operator");
        lhs = n;
    }
    return lhs;
}


static AstNode *parse_shift(Parser *p) {
    AstNode *lhs = parse_add(p);
    while (match_op(p, "<<") || match_op(p, ">>")) {
        Token op = prev(p);
        AstNode *rhs = parse_add(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "%.*s", (int)op.length, op.lexeme);
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        AstNode *folded = fold_binary(n->as.binary_expr.op, lhs, rhs, op.line);
        if (folded) {
            ast_free(n);
            lhs = folded;
            continue;
        }
        n->type = unify_numeric(lhs->type, rhs->type);
        if (n->type == TYPE_UNKNOWN) parse_error(p, op, "invalid types for shift operator");
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_cmp(Parser *p) {
    AstNode *lhs = parse_shift(p);
    while (match_op(p, "<") || match_op(p, ">") || match_op(p, "<=") || match_op(p, ">=") ||
           match_op(p, "==") || match_op(p, "!=")) {
        Token op = prev(p);
        AstNode *rhs = parse_shift(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "%.*s", (int)op.length, op.lexeme);
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        n->type = TYPE_BOOL;
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_bit_and(Parser *p) {
    AstNode *lhs = parse_cmp(p);
    while (match_op(p, "&")) {
        Token op = prev(p);
        AstNode *rhs = parse_cmp(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "&");
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        AstNode *folded = fold_binary("&", lhs, rhs, op.line);
        if (folded) { ast_free(n); lhs = folded; continue; }
        n->type = unify_numeric(lhs->type, rhs->type);
        if (n->type == TYPE_UNKNOWN) parse_error(p, op, "invalid types for bitwise '&'");
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_bit_xor(Parser *p) {
    AstNode *lhs = parse_bit_and(p);
    while (match_op(p, "^")) {
        Token op = prev(p);
        AstNode *rhs = parse_bit_and(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "^");
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        AstNode *folded = fold_binary("^", lhs, rhs, op.line);
        if (folded) { ast_free(n); lhs = folded; continue; }
        n->type = unify_numeric(lhs->type, rhs->type);
        if (n->type == TYPE_UNKNOWN) parse_error(p, op, "invalid types for bitwise '^'");
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_bit_or(Parser *p) {
    AstNode *lhs = parse_bit_xor(p);
    while (match_op(p, "|")) {
        Token op = prev(p);
        AstNode *rhs = parse_bit_xor(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "|");
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        AstNode *folded = fold_binary("|", lhs, rhs, op.line);
        if (folded) { ast_free(n); lhs = folded; continue; }
        n->type = unify_numeric(lhs->type, rhs->type);
        if (n->type == TYPE_UNKNOWN) parse_error(p, op, "invalid types for bitwise '|'");
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_logic_and(Parser *p) {
    AstNode *lhs = parse_bit_or(p);
    while (match_op(p, "&&")) {
        Token op = prev(p);
        AstNode *rhs = parse_bit_or(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "&&");
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        n->type = TYPE_BOOL;
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_logic_or(Parser *p) {
    AstNode *lhs = parse_logic_and(p);
    while (match_op(p, "||")) {
        Token op = prev(p);
        AstNode *rhs = parse_logic_and(p);
        AstNode *n = ast_new(AST_BINARY, op.line);
        snprintf(n->as.binary_expr.op, sizeof(n->as.binary_expr.op), "||");
        n->as.binary_expr.lhs = lhs;
        n->as.binary_expr.rhs = rhs;
        n->type = TYPE_BOOL;
        lhs = n;
    }
    return lhs;
}

static AstNode *parse_assign(Parser *p) {
    AstNode *lhs = parse_logic_or(p);
    if (match_op(p, "=") || match_op(p, "+=") || match_op(p, "-=") || match_op(p, "*=") || match_op(p, "/=") ||
        match_op(p, "%=") || match_op(p, "&=") || match_op(p, "|=") || match_op(p, "^=") ||
        match_op(p, "<<=") || match_op(p, ">>=")) {
        Token eq = prev(p);
        AstNode *rhs = parse_assign(p);

        const char *compound_op = NULL;
        if (eq.length == 2 && eq.lexeme[1] == '=') {
            if (eq.lexeme[0] == '+') compound_op = "+";
            else if (eq.lexeme[0] == '-') compound_op = "-";
            else if (eq.lexeme[0] == '*') compound_op = "*";
            else if (eq.lexeme[0] == '/') compound_op = "/";
            else if (eq.lexeme[0] == '%') compound_op = "%";
            else if (eq.lexeme[0] == '&') compound_op = "&";
            else if (eq.lexeme[0] == '|') compound_op = "|";
            else if (eq.lexeme[0] == '^') compound_op = "^";
        } else if (eq.length == 3 && eq.lexeme[2] == '=') {
            if (eq.lexeme[0] == '<' && eq.lexeme[1] == '<') compound_op = "<<";
            else if (eq.lexeme[0] == '>' && eq.lexeme[1] == '>') compound_op = ">>";
        }

        if (lhs->kind == AST_IDENTIFIER) {
            AstNode *value = rhs;
            if (compound_op) {
                AstNode *lhs_val = ast_new(AST_IDENTIFIER, eq.line);
                lhs_val->as.ident_expr.name = xstrdup(lhs->as.ident_expr.name);
                lhs_val->type = lhs->type;

                AstNode *bin = ast_new(AST_BINARY, eq.line);
                snprintf(bin->as.binary_expr.op, sizeof(bin->as.binary_expr.op), "%s", compound_op);
                bin->as.binary_expr.lhs = lhs_val;
                bin->as.binary_expr.rhs = rhs;

                if (strcmp(compound_op, "+") == 0 || strcmp(compound_op, "-") == 0) {
                    bin->type = unify_numeric(lhs_val->type, rhs->type);
                    if (bin->type == TYPE_UNKNOWN) {
                        bool lhs_ptr = (lhs_val->type == TYPE_PTR);
                        bool rhs_ptr = (rhs->type == TYPE_PTR);
                        bool lhs_num = numeric(lhs_val->type);
                        bool rhs_num = numeric(rhs->type);
                        if (strcmp(compound_op, "+") == 0) {
                            if ((lhs_ptr && rhs_num) || (rhs_ptr && lhs_num)) bin->type = TYPE_PTR;
                        } else {
                            if (lhs_ptr && rhs_num) bin->type = TYPE_PTR;
                        }
                    }
                } else {
                    bin->type = unify_numeric(lhs_val->type, rhs->type);
                }

                if (bin->type == TYPE_UNKNOWN) {
                    parse_error(p, eq, "invalid types for compound assignment");
                }
                value = bin;
            }

            AstNode *n = ast_new(AST_ASSIGN, eq.line);
            n->as.assign_expr.target = xstrdup(lhs->as.ident_expr.name);
            n->as.assign_expr.value = value;
            n->type = value->type;
            ast_free(lhs);
            return n;
        }

        if (compound_op) {
            parse_error(p, eq, "compound assignment currently supports identifiers only");
            ast_free(rhs);
            return lhs;
        }

        if (lhs->kind == AST_UNARY && strcmp(lhs->as.unary_expr.op, "*") == 0) {
            AstNode *n = ast_new(AST_PTR_ASSIGN, eq.line);
            n->as.ptr_assign_expr.ptr = lhs->as.unary_expr.expr;
            lhs->as.unary_expr.expr = NULL;
            n->as.ptr_assign_expr.value = rhs;
            n->type = rhs->type;
            ast_free(lhs);
            return n;
        }
        if (lhs->kind == AST_FIELD) {
            AstNode *n = ast_new(AST_FIELD_ASSIGN, eq.line);
            n->as.field_assign_expr.base = lhs->as.field_expr.base;
            lhs->as.field_expr.base = NULL;
            n->as.field_assign_expr.offset = lhs->as.field_expr.offset;
            n->as.field_assign_expr.through_ptr = lhs->as.field_expr.through_ptr;
            n->as.field_assign_expr.value = rhs;
            n->type = rhs->type;
            ast_free(lhs);
            return n;
        }
        parse_error(p, eq, "left hand side of assignment must be identifier or *pointer");
    }
    return lhs;
}

static AstNode *parse_expr(Parser *p) { return parse_assign(p); }

static AstNode *parse_var_decl(Parser *p, TypeKind ty) {
    Token name_t = consume(p, TOK_IDENTIFIER, "expected identifier for variable declaration");
    AstNode *d = ast_new(AST_VAR_DECL, name_t.line);
    d->as.var_decl.name = dup_lexeme(name_t);
    d->as.var_decl.decl_type = ty;
    d->type = ty;

    Symbol sym = {0};
    sym.name = xstrdup(d->as.var_decl.name);
    sym.type = ty;
    if (p->pending_struct_name[0]) sym.struct_name = xstrdup(p->pending_struct_name);
    sym.is_function = false;
    sym.referenced = false;
    sym_insert(p->scope, sym);

    if (match_op(p, "=")) {
        d->as.var_decl.init = parse_expr(p);
        if (d->as.var_decl.init && numeric(ty) && !numeric(d->as.var_decl.init->type)) {
            parse_error(p, name_t, "numeric variable initialized with non-numeric expression");
        }
    }

    if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after variable declaration");
    return d;
}

static AstNode *parse_if(Parser *p) {
    Token t = prev(p);
    AstNode *n = ast_new(AST_IF, t.line);
    if (!match_symbol(p, '(')) parse_error(p, peek(p), "expected '(' after if");
    n->as.if_stmt.cond = parse_expr(p);
    if (!(numeric(n->as.if_stmt.cond->type) || n->as.if_stmt.cond->type == TYPE_BOOL)) {
        parse_error(p, t, "if condition must be numeric/bool");
    }
    if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after condition");
    n->as.if_stmt.then_branch = parse_statement(p);
    if (match_kw(p, KW_ELSE)) n->as.if_stmt.else_branch = parse_statement(p);

    if (n->as.if_stmt.cond->kind == AST_INT_LITERAL) {
        if (n->as.if_stmt.cond->as.int_lit.value == 0 && n->as.if_stmt.else_branch) {
            AstNode *keep = n->as.if_stmt.else_branch;
            n->as.if_stmt.else_branch = NULL;
            ast_free(n->as.if_stmt.then_branch);
            n->as.if_stmt.then_branch = keep;
        }
    }
    return n;
}

static AstNode *parse_while(Parser *p) {
    Token t = prev(p);
    AstNode *n = ast_new(AST_WHILE, t.line);
    if (!match_symbol(p, '(')) parse_error(p, peek(p), "expected '(' after while");
    n->as.while_stmt.cond = parse_expr(p);
    if (!(numeric(n->as.while_stmt.cond->type) || n->as.while_stmt.cond->type == TYPE_BOOL)) {
        parse_error(p, t, "while condition must be numeric/bool");
    }
    if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after condition");
    p->loop_depth++;
    n->as.while_stmt.body = parse_statement(p);
    p->loop_depth--;
    return n;
}

static AstNode *parse_for(Parser *p) {
    Token t = prev(p);
    AstNode *n = ast_new(AST_FOR, t.line);
    if (!match_symbol(p, '(')) parse_error(p, peek(p), "expected '(' after for");

    if (!match_symbol(p, ';')) {
        n->as.for_stmt.init = parse_expr(p);
        if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after for init");
    }
    if (!match_symbol(p, ';')) {
        n->as.for_stmt.cond = parse_expr(p);
        if (!(numeric(n->as.for_stmt.cond->type) || n->as.for_stmt.cond->type == TYPE_BOOL)) {
            parse_error(p, t, "for condition must be numeric/bool");
        }
        if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after for cond");
    }
    if (!match_symbol(p, ')')) {
        n->as.for_stmt.post = parse_expr(p);
        if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after for post");
    }

    p->loop_depth++;
    n->as.for_stmt.body = parse_statement(p);
    p->loop_depth--;
    return n;
}

static AstNode *parse_switch(Parser *p) {
    Token t = prev(p);
    AstNode *n = ast_new(AST_SWITCH, t.line);
    if (!match_symbol(p, '(')) parse_error(p, peek(p), "expected '(' after switch");
    n->as.switch_stmt.expr = parse_expr(p);
    if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after switch expr");
    if (!match_symbol(p, '{')) parse_error(p, peek(p), "expected '{' for switch body");

    while (!match_symbol(p, '}') && !at_end(p)) {
        if (match_kw(p, KW_CASE)) {
            AstNode *c = ast_new(AST_CASE, prev(p).line);
            c->as.case_stmt.value = parse_expr(p);
            if (!match_symbol(p, ':')) parse_error(p, peek(p), "expected ':' after case value");
            c->as.case_stmt.body = ast_new(AST_BLOCK, peek(p).line);
            while (!(peek(p).kind == TOK_KEYWORD && (peek(p).keyword == KW_CASE || peek(p).keyword == KW_DEFAULT)) &&
                   !(peek(p).kind == TOK_SYMBOL && peek(p).lexeme[0] == '}') && !at_end(p)) {
                node_array_push(&c->as.case_stmt.body->as.block.statements, parse_statement(p));
            }
            node_array_push(&n->as.switch_stmt.cases, c);
        } else if (match_kw(p, KW_DEFAULT)) {
            if (!match_symbol(p, ':')) parse_error(p, peek(p), "expected ':' after default");
            AstNode *b = ast_new(AST_BLOCK, peek(p).line);
            while (!(peek(p).kind == TOK_SYMBOL && peek(p).lexeme[0] == '}') && !at_end(p)) {
                node_array_push(&b->as.block.statements, parse_statement(p));
            }
            n->as.switch_stmt.default_case = b;
        } else {
            parse_error(p, peek(p), "expected case/default in switch");
            p->current++;
        }
    }
    return n;
}

static AstNode *parse_statement(Parser *p) {
    if (match_symbol(p, '{')) return parse_block(p);
    if (match_kw(p, KW_IF)) return parse_if(p);
    if (match_kw(p, KW_WHILE)) return parse_while(p);
    if (match_kw(p, KW_FOR)) return parse_for(p);
    if (match_kw(p, KW_SWITCH)) return parse_switch(p);

    if (match_kw(p, KW_RETURN)) {
        AstNode *n = ast_new(AST_RETURN, prev(p).line);
        if (!match_symbol(p, ';')) {
            n->as.return_stmt.expr = parse_expr(p);
            if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after return");
            if (p->current_function_return != TYPE_VOID && n->as.return_stmt.expr &&
                !numeric(unify_numeric(p->current_function_return, n->as.return_stmt.expr->type)) &&
                p->current_function_return != n->as.return_stmt.expr->type) {
                parse_error(p, prev(p), "return type mismatch");
            }
        }
        return n;
    }

    if (match_kw(p, KW_BREAK)) {
        AstNode *n = ast_new(AST_BREAK, prev(p).line);
        if (p->loop_depth <= 0) parse_error(p, prev(p), "break outside loop/switch");
        if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after break");
        return n;
    }

    if (match_kw(p, KW_CONTINUE)) {
        AstNode *n = ast_new(AST_CONTINUE, prev(p).line);
        if (p->loop_depth <= 0) parse_error(p, prev(p), "continue outside loop");
        if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after continue");
        return n;
    }

    Token t = peek(p);
    if (t.kind == TOK_KEYWORD &&
        (t.keyword == KW_INT32 || t.keyword == KW_INT64 || t.keyword == KW_BYTE ||
         t.keyword == KW_BOOL || t.keyword == KW_STRING || t.keyword == KW_STRUCT)) {
        TypeKind ty = parse_type(p);
        return parse_var_decl(p, ty);
    }

    AstNode *n = ast_new(AST_EXPR_STMT, t.line);
    n->as.expr_stmt.expr = parse_expr(p);
    if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after expression statement");
    return n;
}

static AstNode *parse_block(Parser *p) {
    AstNode *b = ast_new(AST_BLOCK, prev(p).line);
    SymbolTable *old = p->scope;
    p->scope = scope_push(old);

    bool seen_return = false;
    while (!match_symbol(p, '}') && !at_end(p)) {
        AstNode *st = parse_statement(p);
        if (!seen_return) {
            node_array_push(&b->as.block.statements, st);
            if (st->kind == AST_RETURN) seen_return = true;
        } else {
            ast_free(st); /* dead code elimination inside basic block */
        }
    }

    SymbolTable *done = p->scope;
    p->scope = old;
    scope_free(done);
    return b;
}

static AstNode *parse_function(Parser *p, TypeKind rt, Token name_t) {
    AstNode *fn = ast_new(AST_FUNCTION, name_t.line);
    fn->as.function.name = dup_lexeme(name_t);
    fn->as.function.return_type = rt;
    fn->type = rt;

    Symbol fsym = {0};
    fsym.name = xstrdup(fn->as.function.name);
    fsym.is_function = true;
    fsym.return_type = rt;
    fsym.type = rt;
    fsym.referenced = false;
    Symbol *stored = sym_insert(p->scope, fsym);

    if (!match_symbol(p, '(')) parse_error(p, peek(p), "expected '(' after function name");

    SymbolTable *old = p->scope;
    p->scope = scope_push(old);

    while (!match_symbol(p, ')') && !at_end(p)) {
        TypeKind pty = parse_type(p);
        Token pt = consume(p, TOK_IDENTIFIER, "expected parameter name");
        AstNode *param = ast_new(AST_PARAM, pt.line);
        param->as.param.name = dup_lexeme(pt);
        param->as.param.param_type = pty;
        param->type = pty;
        node_array_push(&fn->as.function.params, param);

        Symbol ps = {0};
        ps.name = xstrdup(param->as.param.name);
        ps.type = pty;
        if (p->pending_struct_name[0]) ps.struct_name = xstrdup(p->pending_struct_name);
        ps.is_function = false;
        ps.referenced = true;
        sym_insert(p->scope, ps);

        if (stored->arity < 16) stored->params[stored->arity++] = pty;

        if (!match_symbol(p, ',')) {
            if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after params");
            break;
        }
    }

    p->current_function_return = rt;
    if (match_symbol(p, '{')) {
        fn->as.function.body = parse_block(p);
    } else {
        parse_error(p, peek(p), "expected function body");
    }

    p->scope = old;

    if (fn->as.function.body && fn->as.function.body->kind == AST_BLOCK &&
        fn->as.function.body->as.block.statements.count == 1 &&
        fn->as.function.body->as.block.statements.items[0]->kind == AST_RETURN) {
        fn->as.function.can_inline = true;
    }

    return fn;
}

static AstNode *parse_import(Parser *p) {
    Token t = prev(p);
    Token mod = consume(p, TOK_IDENTIFIER, "expected module identifier in import");
    AstNode *n = ast_new(AST_IMPORT, t.line);
    n->as.import_decl.module = dup_lexeme(mod);
    if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after import");

    /* External libraries are modeled through import entries, no core changes required. */
    return n;
}

static AstNode *parse_extern(Parser *p) {
    Token kw = prev(p);
    TypeKind rt = parse_type(p);
    Token name = consume(p, TOK_IDENTIFIER, "expected extern symbol name");

    AstNode *n = ast_new(AST_EXTERN_DECL, kw.line);
    n->as.extern_decl.name = dup_lexeme(name);
    n->as.extern_decl.return_type = rt;

    Symbol sym = {0};
    sym.name = xstrdup(n->as.extern_decl.name);
    sym.is_function = true;
    sym.return_type = rt;
    sym.type = rt;
    sym.is_extern = true;

    if (!match_symbol(p, '(')) parse_error(p, peek(p), "expected '(' after extern name");
    while (!match_symbol(p, ')') && !at_end(p)) {
        if (match_ellipsis(p)) {
            sym.variadic = true;
            if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after variadic extern");
            break;
        }
        TypeKind pty = parse_type(p);
        AstNode *pn = ast_new(AST_PARAM, peek(p).line);
        pn->as.param.param_type = pty;
        pn->as.param.name = xstrdup("_");
        node_array_push(&n->as.extern_decl.params, pn);
        if (sym.arity < 16) sym.params[sym.arity++] = pty;
        if (!match_symbol(p, ',')) {
            if (!match_symbol(p, ')')) parse_error(p, peek(p), "expected ')' after extern params");
            break;
        }
    }
    if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after extern declaration");

    sym_insert(p->scope, sym);
    return n;
}

static void parse_struct_decl(Parser *p) {
    Token name = consume(p, TOK_IDENTIFIER, "expected struct name");
    if (!match_symbol(p, '{')) {
        parse_error(p, peek(p), "expected '{' in struct declaration");
        return;
    }
    if (p->struct_count >= 64) {
        parse_error(p, name, "too many struct declarations");
        return;
    }
    int idx = p->struct_count++;
    size_t n = name.length < sizeof(p->structs[idx].name) - 1 ? name.length : sizeof(p->structs[idx].name) - 1;
    memcpy(p->structs[idx].name, name.lexeme, n);
    p->structs[idx].name[n] = '\0';
    p->structs[idx].field_count = 0;
    p->structs[idx].size = 0;

    while (!match_symbol(p, '}') && !at_end(p)) {
        TypeKind fty = parse_type(p);
        Token fname = consume(p, TOK_IDENTIFIER, "expected field name");
        if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after struct field");
        if (p->structs[idx].field_count >= 32) continue;
        int fi = p->structs[idx].field_count++;
        size_t fn = fname.length < sizeof(p->structs[idx].fields[fi].name) - 1 ? fname.length : sizeof(p->structs[idx].fields[fi].name) - 1;
        memcpy(p->structs[idx].fields[fi].name, fname.lexeme, fn);
        p->structs[idx].fields[fi].name[fn] = '\0';
        p->structs[idx].fields[fi].type = fty;
        int align = type_size(fty);
        p->structs[idx].size = align_to(p->structs[idx].size, align);
        p->structs[idx].fields[fi].offset = p->structs[idx].size;
        p->structs[idx].size += type_size(fty);
    }
    if (!match_symbol(p, ';')) parse_error(p, peek(p), "expected ';' after struct declaration");
}

AstNode *parse_program(Parser *p, const char *src) {
    memset(p, 0, sizeof(*p));
    p->tokens = lexer_tokenize_all(src);
    p->scope = scope_push(NULL);

    AstNode *prog = ast_new(AST_PROGRAM, 1);

    /* Preload standard library symbols from virtual headers. */
    Symbol libs[] = {
        {.name = NULL, .is_function = true, .return_type = TYPE_VOID, .arity = 1, .params = {TYPE_INT64}, .is_imported = true},
        {.name = NULL, .is_function = true, .return_type = TYPE_INT64, .arity = 0, .is_imported = true},
        {.name = NULL, .is_function = true, .return_type = TYPE_INT64, .arity = 1, .params = {TYPE_INT64}, .is_imported = true},
        {.name = NULL, .is_function = true, .return_type = TYPE_VOID, .arity = 1, .params = {TYPE_INT64}, .is_imported = true},
        {.name = NULL, .is_function = true, .return_type = TYPE_INT64, .arity = 2, .params = {TYPE_INT64, TYPE_INT32}, .is_imported = true},
        {.name = NULL, .is_function = true, .return_type = TYPE_INT32, .arity = 3, .params = {TYPE_INT64, TYPE_INT64, TYPE_INT32}, .is_imported = true}
    };
    const char *lib_names[] = {"println", "scanln", "alloc", "free", "tcp_open", "udp_send"};
    for (size_t i = 0; i < sizeof(libs) / sizeof(libs[0]); ++i) libs[i].name = xstrdup(lib_names[i]);
    for (size_t i = 0; i < sizeof(libs) / sizeof(libs[0]); ++i) sym_insert(p->scope, libs[i]);

    while (!at_end(p)) {
        if (peek(p).kind == TOK_KEYWORD && peek(p).keyword == KW_STRUCT &&
            peek_n(p, 1).kind == TOK_IDENTIFIER &&
            peek_n(p, 2).kind == TOK_SYMBOL && peek_n(p, 2).lexeme[0] == '{') {
            p->current++;
            parse_struct_decl(p);
            continue;
        }
        if (match_kw(p, KW_IMPORT)) {
            node_array_push(&prog->as.program.decls, parse_import(p));
            continue;
        }
        if (match_kw(p, KW_EXTERN)) {
            node_array_push(&prog->as.program.decls, parse_extern(p));
            continue;
        }

        TypeKind rt = parse_type(p);
        Token name = consume(p, TOK_IDENTIFIER, "expected declaration identifier");

        if (match_symbol(p, '(')) {
            p->current--;
            node_array_push(&prog->as.program.decls, parse_function(p, rt, name));
        } else {
            p->current--;
            node_array_push(&prog->as.program.decls, parse_var_decl(p, rt));
        }
    }

    /* Dead function elimination marking: keep extern/imported/referenced/main. */
    for (size_t i = 0; i < prog->as.program.decls.count; ++i) {
        AstNode *d = prog->as.program.decls.items[i];
        if (d->kind == AST_FUNCTION) {
            if (strcmp(d->as.function.name, "main") == 0) d->as.function.dead = false;
            else {
                Symbol *s = sym_lookup(p->scope, d->as.function.name);
                d->as.function.dead = s ? !s->referenced : false;
            }
        }
    }

    return prog;
}

void parser_destroy(Parser *p) {
    token_buffer_free(&p->tokens);
    if (p->scope) scope_free(p->scope);
}
