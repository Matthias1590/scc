#pragma once

#include "scc.h"

typedef enum {
    TOKEN_INTLIT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_IDENTIFIER,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_SEMICOLON,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EQ,
    TOKEN_AMPERSAND,
    TOKEN_VOID,
    TOKEN_RETURN,
} token_type_t;

typedef struct {
    token_type_t type;
    union {
        int intlit;
        char identifier[32];
    } as;
} token_t;

bool tokenize(list_t *tokens, sv_t code_view);
void token_print(const token_t *token);
