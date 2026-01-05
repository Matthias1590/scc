#include "scc.h"

typedef struct {
    list_t *nodes;
    lv_t *token_view;
    size_t *result_index;
} parse_ctx_t;

node_ref_t ctx_get_result_ref(parse_ctx_t *ctx) {
    node_ref_t ref = {
        .nodes = ctx->nodes,
        .index = *ctx->result_index,
    };
    return ref;
}

static void ctx_update(parse_ctx_t *ctx, parse_ctx_t *new_ctx, node_t *node) {
    list_push(new_ctx->nodes, node);
    *new_ctx->result_index = new_ctx->nodes->length - 1;
    *ctx = *new_ctx;
}

node_t *node_ref_get(node_ref_t ref) {
    return list_at(ref.nodes, node_t, ref.index);
}

bool node_ref_is_null(node_ref_t ref) {
    return ref.nodes == NULL;
}

void node_print(node_ref_t ref) {
    node_t *node = node_ref_get(ref);

    switch (node->type) {
        case NODE_INTLIT:
            token_print(&node->as.intlit);
            break;
        case NODE_ADD:
            printf("ADD(");
            node_print(node->as.binop.left_ref);
            printf(", ");
            node_print(node->as.binop.right_ref);
            printf(")");
            break;
        case NODE_MULT:
            printf("MULT(");
            node_print(node->as.binop.left_ref);
            printf(", ");
            node_print(node->as.binop.right_ref);
            printf(")");
            break;
        case NODE_SUB:
            printf("SUB(");
            node_print(node->as.binop.left_ref);
            printf(", ");
            node_print(node->as.binop.right_ref);
            printf(")");
            break;
        case NODE_DIV:
            printf("DIV(");
            node_print(node->as.binop.left_ref);
            printf(", ");
            node_print(node->as.binop.right_ref);
            printf(")");
            break;
        case NODE_VAR_DECL:
            printf("VAR_DECL(");
            node_print(node->as.var_decl.type_ref);
            printf(", ");
            printf("IDENTIFIER(%s)", node->as.var_decl.name->as.identifier);
            if (!node_ref_is_null(node->as.var_decl.init_expr_ref)) {
                printf(", ");
                node_print(node->as.var_decl.init_expr_ref);
            }
            printf(")");
            break;
        case NODE_BLOCK:
            printf("BLOCK{");
            for (size_t i = 0; i < node->as.block.length; i++) {
                node_ref_t stmt_ref = *(node_ref_t *)list_at(&node->as.block, node_ref_t, i);
                node_print(stmt_ref);
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
        case NODE_ASSIGNMENT:
            printf("ASSIGNMENT(");
            node_print(node->as.binop.left_ref);
            printf(", ");
            node_print(node->as.binop.right_ref);
            printf(")");
            break;
        case NODE_IDENTIFIER:
            token_print(&node->as.identifier);
            break;
        default:
            unreachable();
    }
}

static bool try_consume_token(parse_ctx_t *ctx, token_type_t expected_type, token_t **token) {
    if (ctx->token_view->length == 0) {
        return false;
    }
    if (lv_at(ctx->token_view, token_t, 0)->type != expected_type) {
        return false;
    }

    if (token) {
        *token = lv_at(ctx->token_view, token_t, 0);
    }

    ctx->token_view->start++;
    ctx->token_view->length--;
    return true;
}

static bool try_consume_type(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (new_ctx.token_view->length == 0) {
        return false;
    }
    
    node_t type_node;

    token_t *token = lv_at(new_ctx.token_view, token_t, 0);
    if (token->type == TOKEN_INT) {
        type_node.type = NODE_INT;
    } else if (token->type == TOKEN_FLOAT) {
        type_node.type = NODE_FLOAT;
    } else if (token->type == TOKEN_VOID) {
        type_node.type = NODE_VOID;
    } else {
        return false;
    }

    new_ctx.token_view->start++;
    new_ctx.token_view->length--;

    list_push(new_ctx.nodes, &type_node);
    *new_ctx.result_index = new_ctx.nodes->length - 1;
    *ctx = new_ctx;

    return true;
}

static bool try_consume_lhs(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *identifier_token;
    if (!try_consume_token(&new_ctx, TOKEN_IDENTIFIER, &identifier_token)) {
        return false;
    }

    node_t var_node = {
        .type = NODE_IDENTIFIER,
        .as.identifier = *identifier_token,
    };
    ctx_update(ctx, &new_ctx, &var_node);

    return true;
}

static bool try_consume_intlit(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *intlit_token;
    if (!try_consume_token(&new_ctx, TOKEN_INTLIT, &intlit_token)) {
        return false;
    }

    node_t node = {
        .type = NODE_INTLIT,
        .as.intlit = *intlit_token
    };
    ctx_update(ctx, &new_ctx, &node);

    return true;
}

static bool try_consume_expr_0(parse_ctx_t *ctx);
static bool try_consume_expr_2(parse_ctx_t *ctx);

static bool try_consume_parens(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, NULL)) {
        return false;
    }
    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        return false;
    }

    ctx_update(ctx, &new_ctx, node_ref_get(ctx_get_result_ref(&new_ctx)));
    return true;
}

static bool try_consume_cast(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, NULL)) {
        return false;
    }

    if (!try_consume_type(&new_ctx)) {
        return false;
    }
    node_ref_t target_type_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        return false;
    }

    if (!try_consume_expr_2(&new_ctx)) {
        return false;
    }
    node_ref_t expr_ref = ctx_get_result_ref(&new_ctx);

    node_t cast_node = {
        .type = NODE_CAST,
        .as.cast.target_type_ref = target_type_ref,
        .as.cast.expr_ref = expr_ref,
    };
    ctx_update(ctx, &new_ctx, &cast_node);

    return true;
}

static bool try_consume_expr_2(parse_ctx_t *ctx) {
    return try_consume_intlit(ctx)
        || try_consume_cast(ctx)
        || try_consume_parens(ctx)
        || try_consume_lhs(ctx);
}

static bool try_consume_mult(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    while (try_consume_token(&new_ctx, TOKEN_STAR, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_2(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t mult_node = {
            .type = NODE_MULT,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &mult_node);
        parsed = true;
    }

    return parsed;
}

static bool try_consume_div(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    while (try_consume_token(&new_ctx, TOKEN_SLASH, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_2(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t div_node = {
            .type = NODE_DIV,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &div_node);
        parsed = true;
    }

    return parsed;
}

static bool try_consume_expr_1(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_expr_2(&new_ctx)) {
        return false;
    }

    while (new_ctx.token_view->length > 0) {
        if (try_consume_mult(&new_ctx)) {
            continue;
        }
        if (try_consume_div(&new_ctx)) {
            continue;
        }
        break;
    }

    return true;
}

static bool try_consume_add(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    while (try_consume_token(&new_ctx, TOKEN_PLUS, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t add_node = {
            .type = NODE_ADD,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &add_node);
        parsed = true;
    }

    return parsed;
}

static bool try_consume_sub(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    while (try_consume_token(&new_ctx, TOKEN_MINUS, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t sub_node = {
            .type = NODE_SUB,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &sub_node);
        parsed = true;
    }

    return parsed;
}

static bool try_consume_expr_0(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_expr_1(&new_ctx)) {
        return false;
    }

    while (new_ctx.token_view->length > 0) {
        if (try_consume_add(&new_ctx)) {
            continue;
        }
        if (try_consume_sub(&new_ctx)) {
            continue;
        }
        break;
    }

    return true;
}

static bool try_consume_var_decl(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_type(&new_ctx)) {
        return false;
    }
    node_ref_t type_ref = ctx_get_result_ref(&new_ctx);

    token_t *identifier_token;
    if (!try_consume_token(&new_ctx, TOKEN_IDENTIFIER, &identifier_token)) {
        return false;
    }

    node_t var_decl_node = {
        .type = NODE_VAR_DECL,
        .as.var_decl.type_ref = type_ref,
        .as.var_decl.name = identifier_token,
    };
    if (try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        ctx_update(ctx, &new_ctx, &var_decl_node);
        return true;
    } else if (try_consume_token(&new_ctx, TOKEN_EQ, NULL)) {
        if (!try_consume_expr_0(&new_ctx)) {
            return false;
        }
        var_decl_node.as.var_decl.init_expr_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
            return false;
        }

        ctx_update(ctx, &new_ctx, &var_decl_node);
        return true;
    }

    return false;
}

static bool try_consume_return(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_token(&new_ctx, TOKEN_RETURN, NULL)) {
        return false;
    }

    node_t ret_node = {
        .type = NODE_RETURN,
    };

    if (try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        ctx_update(ctx, &new_ctx, &ret_node);
        return true;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    ret_node.as.ret.expr_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        return false;
    }

    ctx_update(ctx, &new_ctx, &ret_node);
    return true;
}

static bool try_consume_assignment(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_lhs(ctx)) {
        return false;
    }
    node_ref_t left_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_EQ, NULL)) {
        return false;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        return false;
    }

    node_t assignment_node = {
        .type = NODE_ASSIGNMENT,
        .as.binop.left_ref = left_ref,
        .as.binop.right_ref = right_ref,
    };
    ctx_update(ctx, &new_ctx, &assignment_node);

    return true;
}

static bool try_consume_stmt(parse_ctx_t *ctx) {
    return try_consume_var_decl(ctx)
        || try_consume_assignment(ctx)
        || try_consume_return(ctx);
}

static bool try_consume_block(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_token(&new_ctx, TOKEN_LBRACE, NULL)) {
        return false;
    }

    list_t stmts = { .element_size = sizeof(node_ref_t) };
    while (true) {
        if (!try_consume_stmt(&new_ctx)) {
            break;
        }
        node_ref_t stmt_ref = ctx_get_result_ref(&new_ctx);
        list_push(&stmts, &stmt_ref);
    }

    if (!try_consume_token(&new_ctx, TOKEN_RBRACE, NULL)) {
        list_clear(&stmts);
        return false;
    }

    node_t block_node = {
        .type = NODE_BLOCK,
        .as.block = stmts,
    };
    ctx_update(ctx, &new_ctx, &block_node);

    return true;
}

bool try_consume_function_signature(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_type(&new_ctx)) {
        return false;
    }
    node_ref_t return_type_ref = ctx_get_result_ref(&new_ctx);

    token_t *identifier_token;
    if (!try_consume_token(&new_ctx, TOKEN_IDENTIFIER, &identifier_token)) {
        return false;
    }

    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, NULL)) {
        return false;
    }
    // TODO: Parse parameters
    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        return false;
    }

    node_t func_sig_node = {
        .type = NODE_FUNCTION_SIGNATURE,
        .as.function_signature.return_type_ref = return_type_ref,
        .as.function_signature.name = identifier_token,
    };
    ctx_update(ctx, &new_ctx, &func_sig_node);

    return true;
}

bool try_consume_function(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_function_signature(&new_ctx)) {
        return false;
    }
    node_ref_t func_sig_ref = ctx_get_result_ref(&new_ctx);

    node_t function_node = {
        .type = NODE_FUNCTION,
        .as.function.signature_ref = func_sig_ref,
    };

    if (try_consume_block(&new_ctx)) {
        function_node.as.function.body_ref = ctx_get_result_ref(&new_ctx);
        ctx_update(ctx, &new_ctx, &function_node);
        return true;
    } else if (try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        ctx_update(ctx, &new_ctx, &function_node);
        return true;
    }

    return false;
}

bool parse(list_t *nodes, list_t *tokens, node_ref_t *root_ref) {
    lv_t token_view = lv_from_list(tokens);
    root_ref->nodes = nodes;
    parse_ctx_t ctx = {
        .nodes = nodes,
        .token_view = &token_view,
        .result_index = &root_ref->index,
    };

    return try_consume_function(&ctx);
}
