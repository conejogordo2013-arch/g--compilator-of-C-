#ifndef GEE_LEXER_H
#define GEE_LEXER_H

#include "token.h"
#include <stddef.h>

typedef struct Lexer {
    const char *src;
    size_t len;
    size_t pos;
    int line;
    int col;
} Lexer;

typedef struct TokenBuffer {
    Token *data;
    size_t count;
    size_t capacity;
} TokenBuffer;

void lexer_init(Lexer *lx, const char *src);
Token lexer_next(Lexer *lx);
TokenBuffer lexer_tokenize_all(const char *src);
void token_buffer_free(TokenBuffer *tb);

#endif
