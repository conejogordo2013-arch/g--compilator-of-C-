#ifndef GEE_TOKEN_H
#define GEE_TOKEN_H

#include <stddef.h>

typedef enum TokenKind {
    TOK_EOF = 0,
    TOK_IDENTIFIER,
    TOK_INTEGER,
    TOK_STRING,
    TOK_KEYWORD,
    TOK_OPERATOR,
    TOK_SYMBOL
} TokenKind;

typedef enum Keyword {
    KW_NONE = 0,
    KW_FN,
    KW_RETURN,
    KW_IF,
    KW_ELSE,
    KW_WHILE,
    KW_FOR,
    KW_SWITCH,
    KW_CASE,
    KW_DEFAULT,
    KW_BREAK,
    KW_CONTINUE,
    KW_INT32,
    KW_INT64,
    KW_BYTE,
    KW_VOID,
    KW_IMPORT,
    KW_EXTERN
} Keyword;

typedef struct Token {
    TokenKind kind;
    Keyword keyword;
    const char *lexeme;
    size_t length;
    long long int_value;
    char *str_value;
    int line;
    int col;
} Token;

#endif
