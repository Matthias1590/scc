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

static bool try_consume_charlit(lex_ctx_t *ctx) {
    source_loc_t start_loc = ctx_get_source_loc(ctx);

    if (ctx->code_view->length == 0 || ctx->code_view->string[0] != '\'') {
        return false;
    }

    size_t i = 1;
    bool escape = false;
    while (i < ctx->code_view->length) {
        if (ctx->code_view->string[i] == '\'' && !escape) {
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
        report_error(start_loc, "Unterminated char literal");
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

    if (strlen(unescaped) != 1) {
        report_error(start_loc, "Char literal must be a single character");
    }

    token_t token = { .type = TOKEN_CHARLIT, .source_loc = start_loc, .as.charlit = unescaped[0] };
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
    while (i < ctx->code_view->length && (isalpha(ctx->code_view->string[i]) || ctx->code_view->string[i] == '_' || (i > 0 && isdigit(ctx->code_view->string[i])))) {
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
    } else if (strcmp(buffer, "break") == 0) {
        token.type = TOKEN_BREAK;
    } else if (strcmp(buffer, "continue") == 0) {
        token.type = TOKEN_CONTINUE;
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
    } else if (strcmp(buffer, "for") == 0) {
        token.type = TOKEN_FOR;
    } else if (strcmp(buffer, "unsigned") == 0) {
        token.type = TOKEN_UNSIGNED;
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
        if (ctx->code_view->length >= 2 && ctx->code_view->string[1] == '=') {
            token.type = TOKEN_PLUSEQ;
            sv_consume(ctx->code_view, 1); // consume extra '='
        } else if (ctx->code_view->length >= 2 && ctx->code_view->string[1] == '+') {
            token.type = TOKEN_INC;
            sv_consume(ctx->code_view, 1); // consume extra '+'
        } else {
            token.type = TOKEN_PLUS;
        }
    } else if (ctx->code_view->string[0] == '-') {
        token.type = TOKEN_MINUS;
    } else if (ctx->code_view->string[0] == '[') {
        token.type = TOKEN_LBRACK;
    } else if (ctx->code_view->string[0] == ']') {
        token.type = TOKEN_RBRACK;
    } else if (ctx->code_view->string[0] == '*') {
        token.type = TOKEN_STAR;
    } else if (ctx->code_view->string[0] == '/') {
        token.type = TOKEN_SLASH;
    } else if (ctx->code_view->string[0] == '>') {
        token.type = TOKEN_GT;
    } else if (ctx->code_view->string[0] == '<') {
        if (ctx->code_view->length >= 2 && ctx->code_view->string[1] == '=') {
            token.type = TOKEN_LTE;
            sv_consume(ctx->code_view, 1); // consume extra '='
        } else {
            token.type = TOKEN_LT;
        }
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
         if (ctx->code_view->length >= 2 && ctx->code_view->string[1] == '&') {
            token.type = TOKEN_ANDAND;
            sv_consume(ctx->code_view, 1); // consume extra '&'
        } else {
            token.type = TOKEN_AMPERSAND;
        }
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
    } else if (ctx->code_view->string[0] == '.') {
        if (ctx->code_view->length >= 2 && ctx->code_view->string[1] == '.') {
            if (ctx->code_view->length >= 3 && ctx->code_view->string[2] == '.') {
                token.type = TOKEN_DOTS;
                sv_consume(ctx->code_view, 2);  // consume extra '..'
            } else {
                // TODO: ".." should be parse error?
                return false;
            }
        } else {
            // TODO: Struct/union member access
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
        } else if (try_consume_charlit(&ctx)) {
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
            fprintf(stderr, "INTLIT(%d)", token->as.intlit);
            break;
        case TOKEN_PLUS:
            fprintf(stderr, "PLUS");
            break;
        case TOKEN_MINUS:
            fprintf(stderr, "MINUS");
            break;
        case TOKEN_STAR:
            fprintf(stderr, "STAR");
            break;
        case TOKEN_SLASH:
            fprintf(stderr, "SLASH");
            break;
        case TOKEN_LPAREN:
            fprintf(stderr, "LPAREN");
            break;
        case TOKEN_RPAREN:
            fprintf(stderr, "RPAREN");
            break;
        case TOKEN_IDENTIFIER:
            fprintf(stderr, "IDENTIFIER(%s)", token->as.identifier);
            break;
        case TOKEN_INT:
            fprintf(stderr, "INT");
            break;
        case TOKEN_FLOAT:
            fprintf(stderr, "FLOAT");
            break;
        case TOKEN_SEMICOLON:
            fprintf(stderr, "SEMICOLON");
            break;
        case TOKEN_LBRACE:
            fprintf(stderr, "LBRACE");
            break;
        case TOKEN_RBRACE:
            fprintf(stderr, "RBRACE");
            break;
        case TOKEN_EQ:
            fprintf(stderr, "EQ");
            break;
        case TOKEN_AMPERSAND:
            fprintf(stderr, "AMPERSAND");
            break;
        case TOKEN_COMMA:
            fprintf(stderr, "COMMA");
            break;
        case TOKEN_VOID:
            fprintf(stderr, "VOID");
            break;
        case TOKEN_RETURN:
            fprintf(stderr, "RETURN");
            break;
        case TOKEN_NEQ:
            fprintf(stderr, "NEQ");
            break;
        case TOKEN_CHAR:
            fprintf(stderr, "CHAR");
            break;
        case TOKEN_IF:
            fprintf(stderr, "IF");
            break;
        case TOKEN_STRINGLIT:
            fprintf(stderr, "STRINGLIT(\"%.*s\")", (int)token->as.stringlit.length, token->as.stringlit.string);
            break;
        case TOKEN_LONG:
            fprintf(stderr, "LONG");
            break;
        case TOKEN_WHILE:
            fprintf(stderr, "WHILE");
            break;
        case TOKEN_CHARLIT:
            fprintf(stderr, "CHARLIT('%c')", token->as.charlit);
            break;
        case TOKEN_GT:
            fprintf(stderr, "GT");
            break;
        case TOKEN_EQEQ:
            fprintf(stderr, "EQEQ");
            break;
        case TOKEN_LT:
            fprintf(stderr, "LT");
            break;
        case TOKEN_LTE:
            fprintf(stderr, "LTE");
            break;
        case TOKEN_PLUSEQ:
            fprintf(stderr, "PLUSEQ");
            break;
        case TOKEN_UNSIGNED:
            fprintf(stderr, "UNSIGNED");
            break;
        case TOKEN_LBRACK:
            fprintf(stderr, "LBRACK");
            break;
        case TOKEN_RBRACK:  
            fprintf(stderr, "RBRACK");
            break;
        case TOKEN_INC:
            fprintf(stderr, "INC");
            break;
        case TOKEN_BREAK:
            fprintf(stderr, "BREAK");
            break;
        case TOKEN_CONTINUE:
            fprintf(stderr, "CONTINUE");
            break;
        case TOKEN_DOTS:
            fprintf(stderr, "DOTS");
            break;
        case TOKEN_ANDAND:
            fprintf(stderr, "ANDAND");
            break;
        case TOKEN_FOR:
            fprintf(stderr, "FOR");
            break;
        default:
            unreachable();
    }
}
