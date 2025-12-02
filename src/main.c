#include <libunwind.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define UNUSED(x) (void)(x)

void print_stack(void) {
    unw_cursor_t cur;
    unw_context_t ctx;
    unw_getcontext(&ctx);
    unw_init_local(&cur, &ctx);

    while (unw_step(&cur) > 0) {
        unw_word_t ip, off;
        char name[256];
        unw_get_reg(&cur, UNW_REG_IP, &ip);
        if (!unw_get_proc_name(&cur, name, sizeof(name), &off))
            printf("0x%lx: %s+0x%lx\n", (long)ip, name, (long)off);
    }
}

#define assert(expr) \
    do { \
        if (!(expr)) { \
            fprintf(stderr, "Assertion failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
            print_stack(); \
            abort(); \
        } \
    } while (0)

#define TODO(msg) \
    do { \
        fprintf(stderr, "TODO: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        print_stack(); \
        abort(); \
    } while (0)

#define UNREACHABLE() \
    do { \
        fprintf(stderr, "UNREACHABLE reached (%s:%d)\n", __FILE__, __LINE__); \
        print_stack(); \
        abort(); \
    } while (0)

typedef struct {
    void *element_bytes;
    size_t element_size;
    size_t length;
    size_t capacity;
} list_t;

void *list_at_raw(list_t *list, size_t index) {
    return (void *)((char *)list->element_bytes + (index * list->element_size));
}

#define list_at(list, type, index) \
    ((type *)list_at_raw(list, index))

void list_push(list_t *list, void *item) {
    if (list->length >= list->capacity) {
        size_t new_capacity = (list->capacity == 0) ? 1 : list->capacity * 2;
        void *new_bytes = realloc(list->element_bytes, new_capacity * list->element_size);
        assert(new_bytes != NULL);
        list->element_bytes = new_bytes;
        list->capacity = new_capacity;
    }
    memcpy(list_at_raw(list, list->length), item, list->element_size);
    list->length++;
}

void list_clear(list_t *list) {
    free(list->element_bytes);
    list->element_bytes = NULL;
    list->length = 0;
    list->capacity = 0;
}

typedef struct {
    list_t *list;
    size_t start;
    size_t length;
} lv_t;

#define lv_at(lv, type, index) \
    ((type *)lv_at_raw(lv, index))

void *lv_at_raw(lv_t *lv, size_t index) {
    return list_at_raw(lv->list, lv->start + index);
}

lv_t lv_from_list(list_t *list) {
    return (lv_t){ .list = list, .start = 0, .length = list->length };
}

typedef struct {
    const char *string;
    size_t length;
} sv_t;

sv_t sv_from_cstr(const char *cstr) {
    return (sv_t){ .string = cstr, .length = strlen(cstr) };
}

sv_t sv_trim_left(sv_t sv) {
    while (sv.length > 0 && isspace(sv.string[0])) {
        sv.string++;
        sv.length--;
    }
    return sv;
}

sv_t sv_take(sv_t sv, size_t n) {
    if (n > sv.length) {
        n = sv.length;
    }
    sv.length = n;
    return sv;
}

sv_t sv_consume(sv_t *sv, size_t n) {
    if (n > sv->length) {
        n = sv->length;
    }
    sv_t result = { .string = sv->string, .length = n };
    sv->string += n;
    sv->length -= n;
    return result;
}

char *sv_to_cstr(sv_t sv, char *buffer, size_t buffer_size) {
    size_t copy_length = (sv.length < buffer_size - 1) ? sv.length : buffer_size - 1;
    memcpy(buffer, sv.string, copy_length);
    buffer[copy_length] = '\0';
    return buffer;
}

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
} token_type_t;

typedef struct {
    token_type_t type;
    union {
        int intlit;
        char identifier[32];
    } as;
} token_t;

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
        default:
            UNREACHABLE();
    }
}

bool tk_try_consume_intlit(list_t *tokens, sv_t *code_view) {
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
        TODO("Report error for integer literal too long");
    }

    sv_to_cstr(intlit_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_INTLIT, .as.intlit = atoi(buffer) };
    list_push(tokens, &token);

    return true;
}

bool tk_try_consume_identifier(list_t *tokens, sv_t *code_view) {
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
        TODO("Make this dynamic");
    }

    sv_to_cstr(identifier_sv, buffer, sizeof(buffer));

    token_t token = { .type = TOKEN_IDENTIFIER };
    if (strcmp(buffer, "int") == 0) {
        token.type = TOKEN_INT;
    } else if (strcmp(buffer, "float") == 0) {
        token.type = TOKEN_FLOAT;
    } else {
        strcpy(token.as.identifier, buffer);
    }
    
    list_push(tokens, &token);

    return true;
}

bool tk_try_consume_symbol(list_t *tokens, sv_t *code_view) {
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

        if (tk_try_consume_intlit(tokens, &code_view)) {
            continue;
        } else if (tk_try_consume_symbol(tokens, &code_view)) {
            continue;
        } else if (tk_try_consume_identifier(tokens, &code_view)) {
            continue;
        }

        TODO("Report error for unknown token");
    }
}

typedef enum {
    NODE_INTLIT,
    NODE_ADD,
    NODE_SUB,
    NODE_MULT,
    NODE_DIV,
    NODE_VAR_DECL,
    NODE_BLOCK,
    NODE_FLOAT,
    NODE_INT,
} node_type_t;

typedef struct {
    node_type_t type;
    union {
        token_t intlit;
        struct {
            size_t left_index;
            size_t right_index;
        } add;
        struct {
            size_t left_index;
            size_t right_index;
        } sub;
        struct {
            size_t left_index;
            size_t right_index;
        } mult;
        struct {
            size_t left_index;
            size_t right_index;
        } div;
        struct {
            size_t type_index;
            token_t *name;
        } var_decl;
        list_t block;
    } as;
} node_t;

void node_print(node_t *node, list_t *nodes) {
    switch (node->type) {
        case NODE_INTLIT:
            token_print(&node->as.intlit);
            break;
        case NODE_ADD:
            printf("ADD(");
            node_print(list_at(nodes, node_t, node->as.add.left_index), nodes);
            printf(", ");
            node_print(list_at(nodes, node_t, node->as.add.right_index), nodes);
            printf(")");
            break;
        case NODE_MULT:
            printf("MULT(");
            node_print(list_at(nodes, node_t, node->as.mult.left_index), nodes);
            printf(", ");
            node_print(list_at(nodes, node_t, node->as.mult.right_index), nodes);
            printf(")");
            break;
        case NODE_SUB:
            printf("SUB(");
            node_print(list_at(nodes, node_t, node->as.sub.left_index), nodes);
            printf(", ");
            node_print(list_at(nodes, node_t, node->as.sub.right_index), nodes);
            printf(")");
            break;
        case NODE_DIV:
            printf("DIV(");
            node_print(list_at(nodes, node_t, node->as.div.left_index), nodes);
            printf(", ");
            node_print(list_at(nodes, node_t, node->as.div.right_index), nodes);
            printf(")");
            break;
        case NODE_VAR_DECL:
            printf("VAR_DECL(");
            node_print(list_at(nodes, node_t, node->as.var_decl.type_index), nodes);
            printf(", ");
            printf("IDENTIFIER(%s)", node->as.var_decl.name->as.identifier);
            printf(")");
            break;
        case NODE_BLOCK:
            printf("BLOCK{");
            for (size_t i = 0; i < node->as.block.length; i++) {
                size_t stmt_index = *(size_t *)list_at(&node->as.block, size_t, i);
                node_print(list_at(nodes, node_t, stmt_index), nodes);
                if (i + 1 < node->as.block.length) {
                    printf(", ");
                }
            }
            printf("}");
            break;
        case NODE_INT:
            printf("INT");
            break;
        case NODE_FLOAT:
            printf("FLOAT");
            break;
        default:
            UNREACHABLE();
    }
}

float node_eval(const node_t *node, list_t *nodes) {
    switch (node->type) {
        case NODE_INTLIT:
            return (float)(node->as.intlit.as.intlit);
        case NODE_ADD:
            return node_eval(list_at(nodes, node_t, node->as.add.left_index), nodes) +
                   node_eval(list_at(nodes, node_t, node->as.add.right_index), nodes);
        case NODE_SUB:
            return node_eval(list_at(nodes, node_t, node->as.sub.left_index), nodes) -
                   node_eval(list_at(nodes, node_t, node->as.sub.right_index), nodes);
        case NODE_MULT:
            return node_eval(list_at(nodes, node_t, node->as.mult.left_index), nodes) *
                   node_eval(list_at(nodes, node_t, node->as.mult.right_index), nodes);
        case NODE_DIV:
            return node_eval(list_at(nodes, node_t, node->as.div.left_index), nodes) /
                   node_eval(list_at(nodes, node_t, node->as.div.right_index), nodes);
        default:
            UNREACHABLE();
    }
}

bool pr_try_consume_intlit(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }

    token_t *token = lv_at(&token_view_copy, token_t, 0);
    if (token->type != TOKEN_INTLIT) {
        return false;
    }

    node_t node = { .type = NODE_INTLIT, .as.intlit = *token };
    list_push(nodes, &node);
    *result_index = nodes->length - 1;

    token_view_copy.start++;
    token_view_copy.length--;

    *token_view = token_view_copy;
    return true;
}

bool try_consume_expr_0(list_t *nodes, lv_t *token_view, size_t *result_index);

bool try_consume_parens(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_LPAREN) {
        return false;
    }

    token_view_copy.start++;
    token_view_copy.length--;

    if (!try_consume_expr_0(nodes, &token_view_copy, result_index)) {
        return false;
    }

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_RPAREN) {
        return false;
    }

    token_view_copy.start++;
    token_view_copy.length--;

    *token_view = token_view_copy;
    return true;
}

bool try_consume_expr_2(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (pr_try_consume_intlit(nodes, &token_view_copy, result_index)) {
        *token_view = token_view_copy;
        return true;
    } else if (try_consume_parens(nodes, &token_view_copy, result_index)) {
        *token_view = token_view_copy;
        return true;
    }

    return false;
}

bool try_consume_mult(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_STAR) {
        return false;
    }

    size_t left_index;
    while (lv_at(&token_view_copy, token_t, 0)->type == TOKEN_STAR) {
        left_index = *result_index;

        token_view_copy.start++;
        token_view_copy.length--;

        if (!try_consume_expr_2(nodes, &token_view_copy, result_index)) {
            return false;
        }

        node_t mult_node = {
            .type = NODE_MULT,
            .as.mult = {
                .left_index = left_index,
                .right_index = *result_index
            }
        };
        list_push(nodes, &mult_node);
        *result_index = nodes->length - 1;
    }

    *token_view = token_view_copy;
    return true;
}

bool try_consume_div(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_SLASH) {
        return false;
    }

    size_t left_index;
    while (lv_at(&token_view_copy, token_t, 0)->type == TOKEN_SLASH) {
        left_index = *result_index;

        token_view_copy.start++;
        token_view_copy.length--;

        if (!try_consume_expr_2(nodes, &token_view_copy, result_index)) {
            return false;
        }

        node_t div_node = {
            .type = NODE_DIV,
            .as.div = {
                .left_index = left_index,
                .right_index = *result_index
            }
        };
        list_push(nodes, &div_node);
        *result_index = nodes->length - 1;
    }

    *token_view = token_view_copy;
    return true;
}

bool try_consume_expr_1(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (!try_consume_expr_2(nodes, &token_view_copy, result_index)) {
        return false;
    }

    while (token_view_copy.length > 0) {
        if (try_consume_mult(nodes, &token_view_copy, result_index)) {
            continue;
        }
        if (try_consume_div(nodes, &token_view_copy, result_index)) {
            continue;
        }
        break;
    }

    *token_view = token_view_copy;
    return true;
}

bool try_consume_add(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_PLUS) {
        return false;
    }

    size_t left_index;
    while (lv_at(&token_view_copy, token_t, 0)->type == TOKEN_PLUS) {
        left_index = *result_index;

        token_view_copy.start++;
        token_view_copy.length--;

        if (!try_consume_expr_1(nodes, &token_view_copy, result_index)) {
            return false;
        }

        node_t add_node = {
            .type = NODE_ADD,
            .as.add = {
                .left_index = left_index,
                .right_index = *result_index
            }
        };
        list_push(nodes, &add_node);
        *result_index = nodes->length - 1;
    }

    *token_view = token_view_copy;
    return true;
}

bool try_consume_sub(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_MINUS) {
        return false;
    }

    size_t left_index;
    while (lv_at(&token_view_copy, token_t, 0)->type == TOKEN_MINUS) {
        left_index = *result_index;

        token_view_copy.start++;
        token_view_copy.length--;

        if (!try_consume_expr_1(nodes, &token_view_copy, result_index)) {
            return false;
        }

        node_t sub_node = {
            .type = NODE_SUB,
            .as.sub = {
                .left_index = left_index,
                .right_index = *result_index
            }
        };
        list_push(nodes, &sub_node);
        *result_index = nodes->length - 1;
    }

    *token_view = token_view_copy;
    return true;
}

bool try_consume_expr_0(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (!try_consume_expr_1(nodes, &token_view_copy, result_index)) {
        return false;
    }

    while (token_view_copy.length > 0) {
        if (try_consume_add(nodes, &token_view_copy, result_index)) {
            continue;
        }
        if (try_consume_sub(nodes, &token_view_copy, result_index)) {
            continue;
        }
        break;
    }

    *token_view = token_view_copy;
    return true;
}

bool try_consume_type(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }
    
    node_t type_node;
    token_t *token = lv_at(&token_view_copy, token_t, 0);
    if (token->type == TOKEN_INT) {
        type_node.type = NODE_INT;
    } else if (token->type == TOKEN_FLOAT) {
        type_node.type = NODE_FLOAT;
    } else {
        return false;
    }

    list_push(nodes, &type_node);
    *result_index = nodes->length - 1;

    token_view_copy.start++;
    token_view_copy.length--;

    *token_view = token_view_copy;
    return true;
}

bool try_consume_var_decl(list_t *nodes, lv_t *token_view, size_t *result_index) {
    UNUSED(nodes);
    UNUSED(result_index);

    lv_t token_view_copy = *token_view;

    size_t type_index;
    if (!try_consume_type(nodes, &token_view_copy, &type_index)) {
        return false;
    }

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_IDENTIFIER) {
        return false;
    }

    token_t *identifier_token = lv_at(&token_view_copy, token_t, 0);

    token_view_copy.start++;
    token_view_copy.length--;

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_SEMICOLON) {
        return false;
    }

    token_view_copy.start++;
    token_view_copy.length--;

    node_t var_decl_node = {
        .type = NODE_VAR_DECL,
        .as.var_decl.type_index = type_index,
        .as.var_decl.name = identifier_token,
    };
    list_push(nodes, &var_decl_node);
    *result_index = nodes->length - 1;

    *token_view = token_view_copy;
    return true;
}

bool try_consume_stmt(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (!try_consume_var_decl(nodes, &token_view_copy, result_index)) {
        return false;
    }

    *token_view = token_view_copy;
    return true;
}

bool try_consume_block(list_t *nodes, lv_t *token_view, size_t *result_index) {
    lv_t token_view_copy = *token_view;

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_LBRACE) {
        return false;
    }

    token_view_copy.start++;
    token_view_copy.length--;

    list_t stmts = { .element_size = sizeof(size_t) };

    while (true) {
        size_t stmt_index;
        if (!try_consume_stmt(nodes, &token_view_copy, &stmt_index)) {
            break;
        }
        list_push(&stmts, &stmt_index);
    }

    if (token_view_copy.length == 0) {
        return false;
    }
    if (lv_at(&token_view_copy, token_t, 0)->type != TOKEN_RBRACE) {
        return false;
    }

    token_view_copy.start++;
    token_view_copy.length--;

    node_t block_node = {
        .type = NODE_BLOCK,
        .as.block = stmts,
    };
    list_push(nodes, &block_node);
    *result_index = nodes->length - 1;

    *token_view = token_view_copy;
    return true;
}

bool parse(list_t *nodes, list_t *tokens, size_t *root_index) {
    lv_t token_view = lv_from_list(tokens);

    return try_consume_block(nodes, &token_view, root_index);
}

char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    assert(file != NULL);

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    assert(buffer != NULL);

    size_t read_size = fread(buffer, 1, file_size, file);
    assert(read_size == (size_t)file_size);
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

int main(void) {
    char *code = read_file("test.c");
    sv_t code_view = sv_from_cstr(code);

    list_t tokens = { .element_size = sizeof(token_t) };
    if (!tokenize(&tokens, code_view)) {
        TODO("Handle tokenization error");
    }

    // for (size_t i = 0; i < tokens.length; i++) {
    //     token_print(list_at(&tokens, token_t, i));
    //     printf("\n");
    // }

    list_t nodes = { .element_size = sizeof(node_t) };
    size_t root;
    if (!parse(&nodes, &tokens, &root)) {
        TODO("Handle parse error");
    }

    assert(nodes.length > 0);
    node_print(list_at(&nodes, node_t, root), &nodes);
    printf("\n");

    // printf("%f\n", node_eval(list_at(&nodes, node_t, root), &nodes));

    free(code);
    list_clear(&nodes);
    list_clear(&tokens);
    return 0;
}
