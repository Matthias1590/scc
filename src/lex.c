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
    source_loc_t start_loc = ctx_get_source_loc(ctx);

    size_t i = 0;
    while (i < ctx->code_view->length && isdigit(ctx->code_view->string[i])) {
        i++;
    }
    if (i == 0) {
        return false;
    }

    sv_t intlit_sv = sv_consume(ctx->code_view, i);

    // TODO: Use sv_t?
    char buffer[32];
    if (intlit_sv.length >= sizeof(buffer)) {
        report_error(start_loc, "Integer literal too long");
    }

    sv_to_cstr(intlit_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_INTLIT, .source_loc = start_loc, .as.intlit = atoi(buffer) };
    list_push(ctx->tokens, &token);

    return true;
}

static bool try_consume_stringlit(lex_ctx_t *ctx) {
    source_loc_t start_loc = ctx_get_source_loc(ctx);

    if (ctx->code_view->length == 0 || ctx->code_view->string[0] != '"') {
        return false;
    }

    size_t i = 1;
    bool escape = false;
    while (i < ctx->code_view->length) {
        if (ctx->code_view->string[i] == '"' && !escape) {
            break;
        }
        if (ctx->code_view->string[i] == '\\' && !escape) {
            escape = true;
        } else {
            escape = false;
        }
        i++;
    }
    if (i >= ctx->code_view->length) {
        report_error(start_loc, "Unterminated string literal");
    }
    i--;

    sv_consume(ctx->code_view, 1); // consume opening quote
    sv_t string_sv = sv_consume(ctx->code_view, i); // exclude closing quote
    sv_consume(ctx->code_view, 1); // consume closing quote

    // Handle escape sequences
    char unescaped[string_sv.length + 1];
    size_t unescaped_index = 0;
    for (size_t j = 0; j < string_sv.length; j++) {
        if (string_sv.string[j] == '\\') {
            j++;
            if (j >= string_sv.length) {
                unreachable();  // TODO: Is this correct? just make sure and remove this todo
            }
            switch (string_sv.string[j]) {
                case 'n':
                    unescaped[unescaped_index++] = '\n';
                    break;
                case 't':
                    unescaped[unescaped_index++] = '\t';
                    break;
                case '\\':
                    unescaped[unescaped_index++] = '\\';
                    break;
                case '"':
                    unescaped[unescaped_index++] = '"';
                    break;
                default:
                    report_error(start_loc, "Unknown escape sequence: \\%c", string_sv.string[j]);
            }
        } else {
            unescaped[unescaped_index++] = string_sv.string[j];
        }
    }
    unescaped[unescaped_index] = '\0';

    string_sv = (sv_t){ .string = strdup(unescaped), .length = strlen(unescaped) };

    token_t token = { .type = TOKEN_STRINGLIT, .source_loc = start_loc, .as.stringlit = string_sv };
    list_push(ctx->tokens, &token);

    return true;
}

static bool try_consume_identifier(lex_ctx_t *ctx) {
    source_loc_t start_loc = ctx_get_source_loc(ctx);

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
        report_error(start_loc, "Identifier too long");  // TODO: Shouldn't be an error
    }

    sv_to_cstr(identifier_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_IDENTIFIER, .source_loc = start_loc };
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
    } else if (strcmp(buffer, "long") == 0) {
        token.type = TOKEN_LONG;
    } else if (strcmp(buffer, "if") == 0) {
        token.type = TOKEN_IF;
    } else if (strcmp(buffer, "else") == 0) {
        token.type = TOKEN_ELSE;
    } else if (strcmp(buffer, "while") == 0) {
        token.type = TOKEN_WHILE;
    } else {
        strcpy(token.as.identifier, buffer);
    }

    list_push(ctx->tokens, &token);
    return true;
}

static bool try_consume_symbol(lex_ctx_t *ctx) {
    source_loc_t start_loc = ctx_get_source_loc(ctx);
    token_t token = { .source_loc = start_loc };

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
        if (ctx->code_view->length >= 2 && ctx->code_view->string[1] == '=') {
            token.type = TOKEN_EQEQ;
            sv_consume(ctx->code_view, 1); // consume extra '='
        } else {
            token.type = TOKEN_EQ;
        }
    } else if (ctx->code_view->string[0] == '&') {
        token.type = TOKEN_AMPERSAND;
    } else if (ctx->code_view->string[0] == ',') {
        token.type = TOKEN_COMMA;
    } else if (ctx->code_view->string[0] == '!') {
        if (ctx->code_view->length >= 2 && ctx->code_view->string[1] == '=') {
            token.type = TOKEN_NEQ;
            sv_consume(ctx->code_view, 1); // consume extra '='
        } else {
            // TODO: Unary NOT token
            return false;
        }
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
        } else if (try_consume_stringlit(&ctx)) {
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
        case TOKEN_AMPERSAND:
            printf("AMPERSAND");
            break;
        case TOKEN_COMMA:
            printf("COMMA");
            break;
        case TOKEN_VOID:
            printf("VOID");
            break;
        case TOKEN_RETURN:
            printf("RETURN");
            break;
        case TOKEN_NEQ:
            printf("NEQ");
            break;
        case TOKEN_CHAR:
            printf("CHAR");
            break;
        case TOKEN_IF:
            printf("IF");
            break;
        case TOKEN_STRINGLIT:
            printf("STRINGLIT(\"%.*s\")", (int)token->as.stringlit.length, token->as.stringlit.string);
            break;
        case TOKEN_LONG:
            printf("LONG");
            break;
        case TOKEN_WHILE:
            printf("WHILE");
            break;
        default:
            unreachable();
    }
}
