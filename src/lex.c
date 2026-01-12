#include "scc.h"

typedef struct {
    list_t *tokens;
    sv_t *code_view;
    const char *code;
    const char *file_name;
} lex_ctx_t;

source_loc_t ctx_get_source_loc(const lex_ctx_t *ctx) {
    size_t line = 1;
    size_t column = 1;
    const char *p = ctx->code;
    while (p < ctx->code_view->string) {
        if (*p == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        p++;
    }
    return (source_loc_t){ .file_name = ctx->file_name, .line = line, .column = column };
}

static bool try_consume_intlit(lex_ctx_t *ctx) {
    size_t i = 0;
    while (i < ctx->code_view->length && isdigit(ctx->code_view->string[i])) {
        i++;
    }
    if (i == 0) {
        return false;
    }

    sv_t intlit_sv = sv_consume(ctx->code_view, i);

    char buffer[32];
    if (intlit_sv.length >= sizeof(buffer)) {
        report_error(ctx_get_source_loc(ctx), "Integer literal too long");
    }

    sv_to_cstr(intlit_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_INTLIT, .as.intlit = atoi(buffer) };
    list_push(ctx->tokens, &token);

    return true;
}

static bool try_consume_identifier(lex_ctx_t *ctx) {
    size_t i = 0;
    while (i < ctx->code_view->length && isalpha(ctx->code_view->string[i])) {
        i++;
    }
    if (i == 0) {
        return false;
    }

    sv_t identifier_sv = sv_consume(ctx->code_view, i);

    char buffer[32];
    if (identifier_sv.length >= sizeof(buffer)) {
        report_error(ctx_get_source_loc(ctx), "Identifier too long");  // TODO: Shouldn't be an error
    }

    sv_to_cstr(identifier_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_IDENTIFIER };
    if (strcmp(buffer, "int") == 0) {
        token.type = TOKEN_INT;
    } else if (strcmp(buffer, "float") == 0) {
        token.type = TOKEN_FLOAT;
    } else if (strcmp(buffer, "void") == 0) {
        token.type = TOKEN_VOID;
    } else if (strcmp(buffer, "return") == 0) {
        token.type = TOKEN_RETURN;
    } else if (strcmp(buffer, "char") == 0) {
        token.type = TOKEN_CHAR;
    } else {
        strcpy(token.as.identifier, buffer);
    }

    list_push(ctx->tokens, &token);

    return true;
}

static bool try_consume_symbol(lex_ctx_t *ctx) {
    token_t token;

    if (ctx->code_view->string[0] == '+') {
        token.type = TOKEN_PLUS;
    } else if (ctx->code_view->string[0] == '-') {
        token.type = TOKEN_MINUS;
    } else if (ctx->code_view->string[0] == '*') {
        token.type = TOKEN_STAR;
    } else if (ctx->code_view->string[0] == '/') {
        token.type = TOKEN_SLASH;
    } else if (ctx->code_view->string[0] == '(') {
        token.type = TOKEN_LPAREN;
    } else if (ctx->code_view->string[0] == ')') {
        token.type = TOKEN_RPAREN;
    } else if (ctx->code_view->string[0] == ';') {
        token.type = TOKEN_SEMICOLON;
    } else if (ctx->code_view->string[0] == '{') {
        token.type = TOKEN_LBRACE;
    } else if (ctx->code_view->string[0] == '}') {
        token.type = TOKEN_RBRACE;
    } else if (ctx->code_view->string[0] == '=') {
        token.type = TOKEN_EQ;
    } else if (ctx->code_view->string[0] == '&') {
        token.type = TOKEN_AMPERSAND;
    } else if (ctx->code_view->string[0] == ',') {
        token.type = TOKEN_COMMA;
    } else {
        return false;
    }

    sv_consume(ctx->code_view, 1);
    list_push(ctx->tokens, &token);
    return true;
}

bool tokenize(list_t *tokens, sv_t code_view, const char *file_name) {
    lex_ctx_t ctx = {
        .tokens = tokens,
        .code_view = &code_view,
        .code = code_view.string,
        .file_name = file_name,
    };

    while (true) {
        code_view = sv_trim_left(code_view);
        ctx.code_view = &code_view;

        if (code_view.length == 0) {
            return true;
        }

        if (try_consume_intlit(&ctx)) {
            continue;
        } else if (try_consume_symbol(&ctx)) {
            continue;
        } else if (try_consume_identifier(&ctx)) {
            continue;
        }

        report_error(ctx_get_source_loc(&ctx), "Unexpected character: '%c'", code_view.string[0]);
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
