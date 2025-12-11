#include "scc.h"

static bool try_consume_intlit(list_t *tokens, sv_t *code_view) {
    size_t i = 0;
    while (i < code_view->length && isdigit(code_view->string[i])) {
        i++;
    }
    if (i == 0) {
        return false;
    }

    sv_t intlit_sv = sv_consume(code_view, i);

    char buffer[32];
    if (intlit_sv.length >= sizeof(buffer)) {
        todo("Report error for integer literal too long");
    }

    sv_to_cstr(intlit_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_INTLIT, .as.intlit = atoi(buffer) };
    list_push(tokens, &token);

    return true;
}

static bool try_consume_identifier(list_t *tokens, sv_t *code_view) {
    size_t i = 0;
    while (i < code_view->length && isalpha(code_view->string[i])) {
        i++;
    }
    if (i == 0) {
        return false;
    }

    sv_t identifier_sv = sv_consume(code_view, i);

    char buffer[32];
    if (identifier_sv.length >= sizeof(buffer)) {
        todo("Make this dynamic");
    }

    sv_to_cstr(identifier_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_IDENTIFIER };
    if (strcmp(buffer, "int") == 0) {
        token.type = TOKEN_INT;
    } else if (strcmp(buffer, "float") == 0) {
        token.type = TOKEN_FLOAT;
    } else if (strcmp(buffer, "void") == 0) {
        token.type = TOKEN_VOID;
    } else {
        strcpy(token.as.identifier, buffer);
    }

    list_push(tokens, &token);

    return true;
}

static bool try_consume_symbol(list_t *tokens, sv_t *code_view) {
    token_t token;

    if (code_view->string[0] == '+') {
        token.type = TOKEN_PLUS;
    } else if (code_view->string[0] == '-') {
        token.type = TOKEN_MINUS;
    } else if (code_view->string[0] == '*') {
        token.type = TOKEN_STAR;
    } else if (code_view->string[0] == '/') {
        token.type = TOKEN_SLASH;
    } else if (code_view->string[0] == '(') {
        token.type = TOKEN_LPAREN;
    } else if (code_view->string[0] == ')') {
        token.type = TOKEN_RPAREN;
    } else if (code_view->string[0] == ';') {
        token.type = TOKEN_SEMICOLON;
    } else if (code_view->string[0] == '{') {
        token.type = TOKEN_LBRACE;
    } else if (code_view->string[0] == '}') {
        token.type = TOKEN_RBRACE;
    } else if (code_view->string[0] == '=') {
        token.type = TOKEN_EQ;
    } else {
        return false;
    }

    sv_consume(code_view, 1);
    list_push(tokens, &token);
    return true;
}

bool tokenize(list_t *tokens, sv_t code_view) {
    while (true) {
        code_view = sv_trim_left(code_view);

        if (code_view.length == 0) {
            return true;
        }

        if (try_consume_intlit(tokens, &code_view)) {
            continue;
        } else if (try_consume_symbol(tokens, &code_view)) {
            continue;
        } else if (try_consume_identifier(tokens, &code_view)) {
            continue;
        }

        todo("Report error for unknown token");
    }
}

void token_print(const token_t *token) {
    switch (token->type) {
        case TOKEN_INTLIT:
            printf("INTLIT(%d)", token->as.intlit);
            break;
        case TOKEN_PLUS:
            printf("PLUS");
            break;
        case TOKEN_MINUS:
            printf("MINUS");
            break;
        case TOKEN_STAR:
            printf("STAR");
            break;
        case TOKEN_SLASH:
            printf("SLASH");
            break;
        case TOKEN_LPAREN:
            printf("LPAREN");
            break;
        case TOKEN_RPAREN:
            printf("RPAREN");
            break;
        case TOKEN_IDENTIFIER:
            printf("IDENTIFIER(%s)", token->as.identifier);
            break;
        case TOKEN_INT:
            printf("INT");
            break;
        case TOKEN_FLOAT:
            printf("FLOAT");
            break;
        case TOKEN_SEMICOLON:
            printf("SEMICOLON");
            break;
        case TOKEN_LBRACE:
            printf("LBRACE");
            break;
        case TOKEN_RBRACE:
            printf("RBRACE");
            break;
        case TOKEN_EQ:
            printf("EQ");
            break;
        default:
            unreachable();
    }
}
