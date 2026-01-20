#pragma once

#include "scc.h"

typedef enum {
    TOKEN_INTLIT,
    TOKEN_STRINGLIT,
    TOKEN_CHARLIT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_IDENTIFIER,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_CHAR,
    TOKEN_LONG,
    TOKEN_SEMICOLON,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EQ,
    TOKEN_AMPERSAND,
    TOKEN_COMMA,
    TOKEN_VOID,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_NEQ,
    TOKEN_EQEQ,
    TOKEN_WHILE,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_LTE,
    TOKEN_PLUSEQ,
    TOKEN_UNSIGNED,
} token_type_t;

typedef struct {
    const char *file_name;
    size_t line;
    size_t column;
} source_loc_t;

typedef struct {
    source_loc_t source_loc;
    token_type_t type;
    union {
        int intlit;
        char identifier[32];
        sv_t stringlit;
        char charlit;
    } as;
} token_t;

bool tokenize(list_t *tokens, sv_t code_view, const char *file_name);
void token_print(const token_t *token);
