#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void token_buffer_push(TokenBuffer *tb, Token tok) {
    if (tb->count == tb->capacity) {
        tb->capacity = tb->capacity ? tb->capacity * 2 : 128;
        tb->data = (Token *)realloc(tb->data, tb->capacity * sizeof(Token));
        if (!tb->data) {
            fprintf(stderr, "fatal: out of memory in lexer\n");
            exit(1);
        }
    }
    tb->data[tb->count++] = tok;
}

static Keyword keyword_of(const char *s, size_t n) {
    struct { const char *w; Keyword k; } table[] = {
        {"fn", KW_FN}, {"return", KW_RETURN}, {"if", KW_IF}, {"else", KW_ELSE},
        {"while", KW_WHILE}, {"for", KW_FOR}, {"switch", KW_SWITCH}, {"case", KW_CASE},
        {"default", KW_DEFAULT}, {"break", KW_BREAK}, {"continue", KW_CONTINUE},
        {"int32", KW_INT32}, {"int64", KW_INT64}, {"byte", KW_BYTE}, {"struct", KW_STRUCT}, {"bool", KW_BOOL},
        {"string", KW_STRING}, {"void", KW_VOID},
        {"import", KW_IMPORT}, {"extern", KW_EXTERN}, {"true", KW_TRUE}, {"false", KW_FALSE}
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i) {
        if (strlen(table[i].w) == n && strncmp(s, table[i].w, n) == 0) {
            return table[i].k;
        }
    }
    return KW_NONE;
}

void lexer_init(Lexer *lx, const char *src) {
    lx->src = src;
    lx->len = strlen(src);
    lx->pos = 0;
    lx->line = 1;
    lx->col = 1;
}

static int peek(Lexer *lx) {
    return (lx->pos < lx->len) ? lx->src[lx->pos] : '\0';
}

static int peek2(Lexer *lx) {
    return (lx->pos + 1 < lx->len) ? lx->src[lx->pos + 1] : '\0';
}


static int advance(Lexer *lx) {
    int ch = peek(lx);
    if (ch == '\0') return ch;
    lx->pos++;
    if (ch == '\n') {
        lx->line++;
        lx->col = 1;
    } else {
        lx->col++;
    }
    return ch;
}

static void skip_ws_comments(Lexer *lx) {
    for (;;) {
        int ch = peek(lx);
        if (isspace(ch)) {
            advance(lx);
            continue;
        }
        if (ch == '/' && peek2(lx) == '/') {
            while (peek(lx) && peek(lx) != '\n') advance(lx);
            continue;
        }
        if (ch == '/' && peek2(lx) == '*') {
            advance(lx); advance(lx);
            while (peek(lx)) {
                if (peek(lx) == '*' && peek2(lx) == '/') {
                    advance(lx); advance(lx);
                    break;
                }
                advance(lx);
            }
            continue;
        }
        break;
    }
}

Token lexer_next(Lexer *lx) {
    skip_ws_comments(lx);

    Token t = {0};
    t.line = lx->line;
    t.col = lx->col;

    int ch = peek(lx);
    if (!ch) {
        t.kind = TOK_EOF;
        t.lexeme = "";
        return t;
    }

    if (isalpha(ch) || ch == '_') {
        size_t start = lx->pos;
        while (isalnum(peek(lx)) || peek(lx) == '_') advance(lx);
        t.lexeme = lx->src + start;
        t.length = lx->pos - start;
        t.keyword = keyword_of(t.lexeme, t.length);
        t.kind = (t.keyword == KW_NONE) ? TOK_IDENTIFIER : TOK_KEYWORD;
        return t;
    }

    if (isdigit(ch)) {
        size_t start = lx->pos;
        long long value = 0;
        if (ch == '0' && (peek2(lx) == 'x' || peek2(lx) == 'X')) {
            advance(lx); advance(lx);
            while (isxdigit(peek(lx))) {
                int c = advance(lx);
                value *= 16;
                if (c >= '0' && c <= '9') value += c - '0';
                else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
            }
            t.kind = TOK_INTEGER;
            t.lexeme = lx->src + start;
            t.length = lx->pos - start;
            t.int_value = value;
            return t;
        }
        while (isdigit(peek(lx))) {
            value = value * 10 + (advance(lx) - '0');
        }
        t.kind = TOK_INTEGER;
        t.lexeme = lx->src + start;
        t.length = lx->pos - start;
        t.int_value = value;
        return t;
    }

    if (ch == '\'' && lx->pos + 2 < lx->len) {
        advance(lx);
        int cv = advance(lx);
        if (cv == '\\') {
            int esc = advance(lx);
            cv = (esc == 'n') ? '\n' : (esc == 't' ? '\t' : esc);
        }
        if (peek(lx) == '\'') advance(lx);
        t.kind = TOK_INTEGER;
        t.length = 1;
        t.int_value = cv;
        return t;
    }

    if (ch == '"') {
        advance(lx);
        size_t start = lx->pos;
        while (peek(lx) && peek(lx) != '"') {
            if (peek(lx) == '\\') advance(lx);
            advance(lx);
        }
        size_t n = lx->pos - start;
        char *s = (char *)malloc(n + 1);
        memcpy(s, lx->src + start, n);
        s[n] = '\0';
        if (peek(lx) == '"') advance(lx);
        t.kind = TOK_STRING;
        t.lexeme = lx->src + start;
        t.length = n;
        t.str_value = s;
        return t;
    }

    char a = (char)advance(lx);
    char b = (char)peek(lx);

    t.lexeme = lx->src + lx->pos - 1;
    t.length = 1;

    int c = (char)peek2(lx);
    if ((a == '<' && b == '<' && c == '=') || (a == '>' && b == '>' && c == '=')) {
        advance(lx);
        advance(lx);
        t.length = 3;
        t.kind = TOK_OPERATOR;
        return t;
    }

    if ((a == '=' && b == '=') || (a == '!' && b == '=') || (a == '<' && b == '=') ||
        (a == '>' && b == '=') || (a == '&' && b == '&') || (a == '|' && b == '|') ||
        (a == '+' && b == '+') || (a == '-' && b == '-') ||
        (a == '+' && b == '=') || (a == '-' && b == '=') ||
        (a == '*' && b == '=') || (a == '/' && b == '=') || (a == '%' && b == '=') ||
        (a == '&' && b == '=') || (a == '|' && b == '=') || (a == '^' && b == '=') ||
        (a == '<' && b == '<') || (a == '>' && b == '>') || (a == '-' && b == '>')) {
        advance(lx);
        t.length = 2;
        t.kind = TOK_OPERATOR;
        return t;
    }

    if (strchr("+-*/%=<>!&|^~", a)) {
        t.kind = TOK_OPERATOR;
    } else {
        t.kind = TOK_SYMBOL;
    }

    return t;
}

TokenBuffer lexer_tokenize_all(const char *src) {
    Lexer lx;
    lexer_init(&lx, src);
    TokenBuffer tb = {0};

    for (;;) {
        Token tok = lexer_next(&lx);
        token_buffer_push(&tb, tok);
        if (tok.kind == TOK_EOF) break;
    }

    return tb;
}

void token_buffer_free(TokenBuffer *tb) {
    for (size_t i = 0; i < tb->count; ++i) {
        if (tb->data[i].kind == TOK_STRING) free(tb->data[i].str_value);
    }
    free(tb->data);
    tb->data = NULL;
    tb->count = tb->capacity = 0;
}
