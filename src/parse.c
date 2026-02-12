#include "scc.h"

typedef struct {
    list_t *nodes;
    lv_t token_view;
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
            fprintf(stderr, "ADD(");
            node_print(node->as.binop.left_ref);
            fprintf(stderr, ", ");
            node_print(node->as.binop.right_ref);
            fprintf(stderr, ")");
            break;
        case NODE_MULT:
            fprintf(stderr, "MULT(");
            node_print(node->as.binop.left_ref);
            fprintf(stderr, ", ");
            node_print(node->as.binop.right_ref);
            fprintf(stderr, ")");
            break;
        case NODE_SUB:
            fprintf(stderr, "SUB(");
            node_print(node->as.binop.left_ref);
            fprintf(stderr, ", ");
            node_print(node->as.binop.right_ref);
            fprintf(stderr, ")");
            break;
        case NODE_DIV:
            fprintf(stderr, "DIV(");
            node_print(node->as.binop.left_ref);
            fprintf(stderr, ", ");
            node_print(node->as.binop.right_ref);
            fprintf(stderr, ")");
            break;
        case NODE_VAR_DECL:
            fprintf(stderr, "VAR_DECL(");
            node_print(node->as.var_decl.type_ref);
            fprintf(stderr, ", ");
            fprintf(stderr, "IDENTIFIER(%s)", node->as.var_decl.name->as.identifier);
            if (!node_ref_is_null(node->as.var_decl.init_expr_ref)) {
                fprintf(stderr, ", ");
                node_print(node->as.var_decl.init_expr_ref);
            }
            fprintf(stderr, ")");
            break;
        case NODE_BLOCK:
            fprintf(stderr, "BLOCK{");
            for (size_t i = 0; i < node->as.block.length; i++) {
                node_ref_t stmt_ref = *(node_ref_t *)list_at(&node->as.block, node_ref_t, i);
                node_print(stmt_ref);
                if (i + 1 < node->as.block.length) {
                    fprintf(stderr, ", ");
                }
            }
            fprintf(stderr, "}");
            break;
        case NODE_INT:
            fprintf(stderr, "INT");
            break;
        case NODE_FLOAT:
            fprintf(stderr, "FLOAT");
            break;
        case NODE_ASSIGNMENT:
            fprintf(stderr, "ASSIGNMENT(");
            node_print(node->as.binop.left_ref);
            fprintf(stderr, ", ");
            node_print(node->as.binop.right_ref);
            fprintf(stderr, ")");
            break;
        case NODE_IDENTIFIER:
            token_print(&node->as.identifier);
            break;
        default:
            unreachable();
    }
}

static bool try_consume_stmt(parse_ctx_t *ctx);
static bool try_consume_block(parse_ctx_t *ctx);

static bool try_consume_token(parse_ctx_t *ctx, token_type_t expected_type, token_t **token) {
    if (ctx->token_view.length == 0) {
        return false;
    }
    if (lv_at(&ctx->token_view, token_t, 0)->type != expected_type) {
        return false;
    }

    if (token) {
        *token = lv_at(&ctx->token_view, token_t, 0);
    }

    ctx->token_view.start++;
    ctx->token_view.length--;
    return true;
}

static bool try_consume_type(parse_ctx_t *ctx) {
    trace("+ try_consume_type\n");
    parse_ctx_t new_ctx = *ctx;

    bool is_unsigned = false;
    if (try_consume_token(&new_ctx, TOKEN_UNSIGNED, NULL)) {
        is_unsigned = true;
    }

    if (new_ctx.token_view.length == 0) {
        trace("- try_consume_type: false\n");
        return false;
    }

    node_t type_node = {
        .as.type.is_signed = !is_unsigned,
    };

    token_t *token = lv_at(&new_ctx.token_view, token_t, 0);
    type_node.source_loc = token->source_loc;
    if (token->type == TOKEN_INT) {
        type_node.type = NODE_INT;
    } else if (token->type == TOKEN_FLOAT) {
        type_node.type = NODE_FLOAT;
    } else if (token->type == TOKEN_VOID) {
        type_node.type = NODE_VOID;
    } else if (token->type == TOKEN_CHAR) {
        type_node.type = NODE_CHAR;
    } else if (token->type == TOKEN_LONG) {
        type_node.type = NODE_LONG;
    } else {
        trace("- try_consume_type: false\n");
        return false;
    }

    new_ctx.token_view.start++;
    new_ctx.token_view.length--;

    list_push(new_ctx.nodes, &type_node);
    *new_ctx.result_index = new_ctx.nodes->length - 1;

    token_t *star_token;
    while (try_consume_token(&new_ctx, TOKEN_STAR, &star_token)) {
        node_t ptr_node = {
            .type = NODE_PTR_TYPE,
            .source_loc = star_token->source_loc,
            .as.ptr_type.base_type_ref = ctx_get_result_ref(&new_ctx),
        };
        list_push(new_ctx.nodes, &ptr_node);
        *new_ctx.result_index = new_ctx.nodes->length - 1;
    }

    *ctx = new_ctx;
    trace("- try_consume_type: true\n");
    return true;
}

static bool try_consume_expr_0(parse_ctx_t *ctx);

static bool try_consume_identifier(parse_ctx_t *ctx) {
    trace("+ try_consume_identifier\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *identifier_token;
    if (!try_consume_token(&new_ctx, TOKEN_IDENTIFIER, &identifier_token)) {
        trace("- try_consume_identifier: false\n");
        return false;
    }

    node_t var_node = {
        .type = NODE_IDENTIFIER,
        .source_loc = identifier_token->source_loc,
        .as.identifier = *identifier_token,
    };
    ctx_update(ctx, &new_ctx, &var_node);

    trace("- try_consume_identifier: true\n");
    return true;
}

static bool try_consume_intlit(parse_ctx_t *ctx) {
    trace("+ try_consume_intlit\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *intlit_token;
    if (!try_consume_token(&new_ctx, TOKEN_INTLIT, &intlit_token)) {
        trace("- try_consume_intlit: false\n");
        return false;
    }

    node_t node = {
        .type = NODE_INTLIT,
        .source_loc = intlit_token->source_loc,
        .as.intlit = *intlit_token
    };
    ctx_update(ctx, &new_ctx, &node);

    trace("- try_consume_intlit: true\n");
    return true;
}

static bool try_consume_charlit(parse_ctx_t *ctx) {
    trace("+ try_consume_charlit\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *charlit_token;
    if (!try_consume_token(&new_ctx, TOKEN_CHARLIT, &charlit_token)) {
        trace("- try_consume_charlit: false\n");
        return false;
    }

    node_t node = {
        .type = NODE_CHARLIT,
        .source_loc = charlit_token->source_loc,
        .as.charlit = *charlit_token
    };
    ctx_update(ctx, &new_ctx, &node);

    trace("- try_consume_charlit: true\n");
    return true;
}

static bool try_consume_stringlit(parse_ctx_t *ctx) {
    trace("+ try_consume_stringlit\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *stringlit_token;
    if (!try_consume_token(&new_ctx, TOKEN_STRINGLIT, &stringlit_token)) {
        trace("- try_consume_stringlit: false\n");
        return false;
    }

    node_t node = {
        .type = NODE_STRINGLIT,
        .source_loc = stringlit_token->source_loc,
        .as.stringlit = *stringlit_token
    };
    ctx_update(ctx, &new_ctx, &node);

    trace("- try_consume_stringlit: true\n");
    return true;
}

static bool try_consume_expr_0(parse_ctx_t *ctx);
static bool try_consume_expr_2(parse_ctx_t *ctx);

static bool try_consume_parens(parse_ctx_t *ctx) {
    trace("+ try_consume_parens\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *lpar_token;
    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, &lpar_token)) {
        trace("- try_consume_parens: false\n");
        return false;
    }
    if (!try_consume_expr_0(&new_ctx)) {
        trace("- try_consume_parens: false\n");
        return false;
    }
    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        trace("- try_consume_parens: false\n");
        return false;
    }

    node_t *expr_node = node_ref_get(ctx_get_result_ref(&new_ctx));
    expr_node->source_loc = lpar_token->source_loc;
    ctx_update(ctx, &new_ctx, expr_node);

    trace("- try_consume_parens: true\n");
    return true;
}

static bool try_consume_index(parse_ctx_t *ctx) {
    trace("+ try_consume_index\n");
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_LBRACK, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        node_ref_t index_ref;
        if (!try_consume_expr_0(&new_ctx)) {
            trace("- try_consume_index: false\n");
            return false;
        }
        index_ref = ctx_get_result_ref(&new_ctx);

        node_t index_node = {
            .type = NODE_INDEX,
            .source_loc = source_loc,
            .as.index = {
                .expr_ref = left_ref,
                .index_ref = index_ref,
            }
        };
        ctx_update(ctx, &new_ctx, &index_node);
        parsed = true;
    }

    if (!try_consume_token(&new_ctx, TOKEN_RBRACK, NULL)) {
        trace("- try_consume_index: false\n");
        return false;
    }

    if (parsed) {
        ctx_update(ctx, &new_ctx, node_ref_get(ctx_get_result_ref(&new_ctx)));
        trace("- try_consume_index: true\n");
    } else {
        trace("- try_consume_index: false\n");
    }
    return parsed;
}

static bool try_consume_call(parse_ctx_t *ctx) {
    trace("+ try_consume_call\n");
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_LPAREN, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        list_t arg_refs = { .element_size = sizeof(node_ref_t) };
        while (true) {
            if (try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
                break;
            }
            if (arg_refs.length > 0) {
                if (!try_consume_token(&new_ctx, TOKEN_COMMA, NULL)) {
                    trace("- try_consume_call: false\n");
                    return false;
                }
            }
            if (!try_consume_expr_0(&new_ctx)) {
                trace("- try_consume_call: false\n");
                return false;
            }
            node_ref_t arg_ref = ctx_get_result_ref(&new_ctx);
            list_push(&arg_refs, &arg_ref);
        }

        node_t call_node = {
            .type = NODE_CALL,
            .source_loc = source_loc,
            .as.call = {
                .function_ref = left_ref,
                .arg_refs = arg_refs,
            }
        };
        ctx_update(ctx, &new_ctx, &call_node);
        parsed = true;
    }

    if (parsed) {
        trace("- try_consume_call: true\n");
    } else {
        trace("- try_consume_call: false\n");
    }
    return parsed;
}

static bool try_consume_cast(parse_ctx_t *ctx) {
    trace("+ try_consume_cast\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *lpar_token;
    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, &lpar_token)) {
        trace("- try_consume_cast: false\n");
        return false;
    }

    if (!try_consume_type(&new_ctx)) {
        trace("- try_consume_cast: false\n");
        return false;
    }
    node_ref_t target_type_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        trace("- try_consume_cast: false\n");
        return false;
    }

    if (!try_consume_expr_2(&new_ctx)) {
        trace("- try_consume_cast: false\n");
        return false;
    }
    node_ref_t expr_ref = ctx_get_result_ref(&new_ctx);

    node_t cast_node = {
        .type = NODE_CAST,
        .source_loc = lpar_token->source_loc,
        .as.cast.target_type_ref = target_type_ref,
        .as.cast.expr_ref = expr_ref,
    };
    ctx_update(ctx, &new_ctx, &cast_node);

    trace("- try_consume_cast: true\n");
    return true;
}

static bool try_consume_address_of(parse_ctx_t *ctx) {
    trace("+ try_consume_address_of\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *amp_token;
    if (!try_consume_token(&new_ctx, TOKEN_AMPERSAND, &amp_token)) {
        trace("- try_consume_address_of: false\n");
        return false;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        trace("- try_consume_address_of: false\n");
        return false;
    }
    node_ref_t expr_ref = ctx_get_result_ref(&new_ctx);

    node_t ptr_node = {
        .type = NODE_ADDRESS_OF,
        .source_loc = amp_token->source_loc,
        .as.address_of.expr_ref = expr_ref,
    };
    ctx_update(ctx, &new_ctx, &ptr_node);

    trace("- try_consume_address_of: true\n");
    return true;
}

static bool try_consume_deref(parse_ctx_t *ctx) {
    trace("+ try_consume_deref\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *star_token;
    if (!try_consume_token(&new_ctx, TOKEN_STAR, &star_token)) {
        trace("- try_consume_deref: false\n");
        return false;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        trace("- try_consume_deref: false\n");
        return false;
    }
    node_ref_t expr_ref = ctx_get_result_ref(&new_ctx);

    node_t deref_node = {
        .type = NODE_DEREF,
        .source_loc = star_token->source_loc,
        .as.deref.expr_ref = expr_ref,
    };
    ctx_update(ctx, &new_ctx, &deref_node);

    trace("- try_consume_deref: true\n");
    return true;
}

static bool try_consume_negate(parse_ctx_t *ctx) {
    trace("+ try_consume_negate\n");
    parse_ctx_t new_ctx = *ctx;

    token_t *minus_token;
    if (!try_consume_token(&new_ctx, TOKEN_MINUS, &minus_token)) {
        trace("- try_consume_negate: false\n");
        return false;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        trace("- try_consume_negate: false\n");
        return false;
    }
    node_ref_t expr_ref = ctx_get_result_ref(&new_ctx);

    node_t negate_node = {
        .type = NODE_NEGATE,
        .source_loc = minus_token->source_loc,
        .as.negate.expr_ref = expr_ref,
    };
    ctx_update(ctx, &new_ctx, &negate_node);

    trace("- try_consume_negate: true\n");
    return true;
}

static bool try_consume_expr_3(parse_ctx_t *ctx) {
    trace("| try_consume_expr_3\n");
    return try_consume_deref(ctx)
        || try_consume_negate(ctx)
        || try_consume_address_of(ctx)
        || try_consume_cast(ctx)
        || try_consume_parens(ctx)
        || try_consume_identifier(ctx)
        || try_consume_intlit(ctx)
        || try_consume_stringlit(ctx)
        || try_consume_charlit(ctx);
}

static bool try_consume_postinc(parse_ctx_t *ctx) {
    trace("+ try_consume_postinc\n");
    parse_ctx_t new_ctx = *ctx;

    node_ref_t expr_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_INC, NULL)) {
        trace("- try_consume_postinc: false\n");
        return false;
    }

    node_t postinc_node = {
        .type = NODE_POSTINC,
        .source_loc = node_ref_get(expr_ref)->source_loc,
        .as.postinc.expr_ref = expr_ref,
    };
    ctx_update(ctx, &new_ctx, &postinc_node);

    trace("- try_consume_postinc: true\n");
    return true;
}

static bool try_consume_expr_2(parse_ctx_t *ctx) {
    trace("| try_consume_expr_2\n");

    if (!try_consume_expr_3(ctx)) {
        return false;
    }

    while (ctx->token_view.length > 0) {
        if (try_consume_postinc(ctx)) {
            continue;
        }

        while (ctx->token_view.length > 0) {
            if (try_consume_call(ctx)) {
                continue;
            }
            if (try_consume_index(ctx)) {
                continue;
            }
            break;
        }

        if (try_consume_postinc(ctx)) {
            continue;
        }

        break;
    }

    return true;
}

// TODO: All this new_ctx stuff can be abstracted
// TODO: All this binop parsing can be abstracted
// TODO: Come up with a way to handle precedence with some sort of map or something, not this expr_1, expr_0, expr_3, etc. mess...
static bool try_consume_mult(parse_ctx_t *ctx) {
    trace("+ try_consume_mult\n");
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_STAR, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_2(&new_ctx)) {
            trace("- try_consume_mult: false\n");
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t mult_node = {
            .type = NODE_MULT,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &mult_node);
        parsed = true;
    }

    if (parsed) {
        trace("- try_consume_mult: true\n");
    } else {
        trace("- try_consume_mult: false\n");
    }
    return parsed;
}

static bool try_consume_div(parse_ctx_t *ctx) {
    trace("+ try_consume_div\n");
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_SLASH, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_2(&new_ctx)) {
            trace("- try_consume_div: false\n");
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t div_node = {
            .type = NODE_DIV,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &div_node);
        parsed = true;
    }

    if (parsed) {
        trace("- try_consume_div: true\n");
    } else {
        trace("- try_consume_div: false\n");
    }
    return parsed;
}

static bool try_consume_expr_1(parse_ctx_t *ctx) {
    trace("| try_consume_expr_1\n");

    if (!try_consume_expr_2(ctx)) {
        return false;
    }

    while (ctx->token_view.length > 0) {
        if (try_consume_mult(ctx)) {
            continue;
        }
        if (try_consume_div(ctx)) {
            continue;
        }
        break;
    }

    return true;
}

static bool try_consume_add(parse_ctx_t *ctx) {
    trace("+ try_consume_add\n");
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_PLUS, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            trace("- try_consume_add: false\n");
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t add_node = {
            .type = NODE_ADD,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &add_node);
        parsed = true;
    }

    if (parsed) {
        trace("- try_consume_add: true\n");
    } else {
        trace("- try_consume_add: false\n");
    }
    return parsed;
}

static bool try_consume_sub(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_MINUS, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t sub_node = {
            .type = NODE_SUB,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &sub_node);
        parsed = true;
    }

    if (parsed) {
        trace("try_consume_sub succeeded\n");
    }
    return parsed;
}

static bool try_consume_neq(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_NEQ, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t neq_node = {
            .type = NODE_NEQ,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &neq_node);
        parsed = true;
    }

    if (parsed) {
        trace("try_consume_neq succeeded\n");
    }
    return parsed;
}

static bool try_consume_eqeq(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_EQEQ, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t eqeq_node = {
            .type = NODE_EQEQ,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &eqeq_node);
        parsed = true;
    }

    if (parsed) {
        trace("try_consume_eqeq succeeded\n");
    }
    return parsed;
}

static bool try_consume_gt(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_GT, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t gt_node = {
            .type = NODE_GT,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &gt_node);
        parsed = true;
    }

    if (parsed) {
        trace("try_consume_gt succeeded\n");
    }
    return parsed;
}

static bool try_consume_lt(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_LT, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t lt_node = {
            .type = NODE_LT,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &lt_node);
        parsed = true;
    }

    if (parsed) {
        trace("try_consume_lt succeeded\n");
    }
    return parsed;
}

static bool try_consume_lte(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    bool parsed = false;

    node_ref_t left_ref;
    source_loc_t source_loc = node_ref_get(ctx_get_result_ref(&new_ctx))->source_loc;
    while (try_consume_token(&new_ctx, TOKEN_LTE, NULL)) {
        left_ref = ctx_get_result_ref(&new_ctx);

        if (!try_consume_expr_1(&new_ctx)) {
            return false;
        }
        node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

        node_t lte_node = {
            .type = NODE_LTE,
            .source_loc = source_loc,
            .as.binop = {
                .left_ref = left_ref,
                .right_ref = right_ref
            }
        };
        ctx_update(ctx, &new_ctx, &lte_node);
        parsed = true;
    }

    if (parsed) {
        trace("try_consume_lte succeeded\n");
    }
    return parsed;
}

static bool try_consume_expr_0(parse_ctx_t *ctx) {
    if (!try_consume_expr_1(ctx)) {
        return false;
    }

    while (ctx->token_view.length > 0) {
        if (try_consume_add(ctx)) {
            continue;
        }
        if (try_consume_sub(ctx)) {
            continue;
        }
        if (try_consume_neq(ctx)) {  // TODO: Im pretty sure this operator precedence is wrong
            continue;
        }
        if (try_consume_eqeq(ctx)) {
            continue;
        }
        if (try_consume_gt(ctx)) {
            continue;
        }
        if (try_consume_lt(ctx)) {
            continue;
        }
        if (try_consume_lte(ctx)) {
            continue;
        }
        break;
    }

    trace("try_consume_expr_0 succeeded\n");
    return true;
}

static bool try_consume_array_decl(parse_ctx_t *ctx, node_t *var_decl) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_token(&new_ctx, TOKEN_LBRACK, NULL)) {
        return false;
    }

    node_ref_t expr_ref = {0};
    if (try_consume_expr_0(&new_ctx)) {
        expr_ref = ctx_get_result_ref(&new_ctx);
    }

    if (!try_consume_token(&new_ctx, TOKEN_RBRACK, NULL)) {
        return false;
    }

    ctx_update(ctx, &new_ctx, node_ref_get(ctx_get_result_ref(&new_ctx)));
    *ctx = new_ctx;

    var_decl->as.var_decl.is_array = true;
    var_decl->as.var_decl.array_size_expr_ref = expr_ref;

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
        .source_loc = node_ref_get(type_ref)->source_loc,
        .as.var_decl.type_ref = type_ref,
        .as.var_decl.name = identifier_token,
    };

    try_consume_array_decl(&new_ctx, &var_decl_node);

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

    token_t *return_token;
    if (!try_consume_token(&new_ctx, TOKEN_RETURN, &return_token)) {
        return false;
    }

    node_t ret_node = {
        .type = NODE_RETURN,
        .source_loc = return_token->source_loc,
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

static bool try_consume_while(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *while_token;
    if (!try_consume_token(&new_ctx, TOKEN_WHILE, &while_token)) {
        return false;
    }

    node_t while_node = {
        .type = NODE_WHILE,
        .source_loc = while_token->source_loc,
    };

    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, NULL)) {
        return false;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    while_node.as.while_.expr_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        return false;
    }

    if (!try_consume_stmt(&new_ctx)) {
        return false;
    }
    while_node.as.while_.body_ref = ctx_get_result_ref(&new_ctx);

    ctx_update(ctx, &new_ctx, &while_node);
    return true;
}

static bool try_consume_if(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *if_token;
    if (!try_consume_token(&new_ctx, TOKEN_IF, &if_token)) {
        return false;
    }

    node_t if_node = {
        .type = NODE_IF,
        .source_loc = if_token->source_loc,
    };

    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, NULL)) {
        return false;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    if_node.as.if_.expr_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        return false;
    }

    if (!try_consume_stmt(&new_ctx)) {
        return false;
    }
    if_node.as.if_.then_ref = ctx_get_result_ref(&new_ctx);

    if (try_consume_token(&new_ctx, TOKEN_ELSE, NULL)) {
        if (!try_consume_stmt(&new_ctx)) {
            return false;
        }
        if_node.as.if_.else_ref = ctx_get_result_ref(&new_ctx);
    }

    ctx_update(ctx, &new_ctx, &if_node);
    return true;
}

static bool try_consume_pluseq(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    node_ref_t left_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_PLUSEQ, NULL)) {
        return false;
    }

    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    node_ref_t right_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        return false;
    }

    node_t pluseq_node = {
        .type = NODE_PLUSEQ,
        .source_loc = node_ref_get(left_ref)->source_loc,
        .as.binop.left_ref = left_ref,
        .as.binop.right_ref = right_ref,
    };
    ctx_update(ctx, &new_ctx, &pluseq_node);

    return true;
}

static bool try_consume_assignment(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_expr_0(&new_ctx)) {
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
        .source_loc = node_ref_get(left_ref)->source_loc,
        .as.binop.left_ref = left_ref,
        .as.binop.right_ref = right_ref,
    };
    ctx_update(ctx, &new_ctx, &assignment_node);

    return true;
}

static bool try_consume_discard(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_expr_0(&new_ctx)) {
        return false;
    }
    node_ref_t expr_ref = ctx_get_result_ref(&new_ctx);

    if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        return false;
    }

    node_t discard_node = {
        .type = NODE_DISCARD,
        .source_loc = node_ref_get(expr_ref)->source_loc,
        .as.discard.expr_ref = expr_ref,
    };
    ctx_update(ctx, &new_ctx, &discard_node);
    return true;
}

static bool try_consume_empty_stmt(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        return false;
    }

    node_t empty_stmt_node = {
        .type = NODE_EMPTY_STMT,
        .source_loc = {0},
    };
    ctx_update(ctx, &new_ctx, &empty_stmt_node);
    return true;
}

static bool try_consume_break(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *break_token;
    if (!try_consume_token(&new_ctx, TOKEN_BREAK, &break_token)) {
        return false;
    }

    if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        return false;
    }

    node_t break_node = {
        .type = NODE_BREAK,
        .source_loc = break_token->source_loc,
    };
    ctx_update(ctx, &new_ctx, &break_node);
    return true;
}

static bool try_consume_continue(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *continue_token;
    if (!try_consume_token(&new_ctx, TOKEN_CONTINUE, &continue_token)) {
        return false;
    }

    if (!try_consume_token(&new_ctx, TOKEN_SEMICOLON, NULL)) {
        return false;
    }

    node_t continue_node = {
        .type = NODE_CONTINUE,
        .source_loc = continue_token->source_loc,
    };
    ctx_update(ctx, &new_ctx, &continue_node);
    return true;
}

static bool try_consume_stmt(parse_ctx_t *ctx) {
    if (try_consume_empty_stmt(ctx)) {
        return true;
    }

    if (try_consume_var_decl(ctx)) {
        return true;
    }

    if (try_consume_break(ctx)) {
        return true;
    }

    if (try_consume_continue(ctx)) {
        return true;
    }

    // TODO: This should be an expression, just like assignment, not a statement
    if (try_consume_pluseq(ctx)) {
        return true;
    }

    if (try_consume_assignment(ctx)) {
        return true;
    }

    if (try_consume_return(ctx)) {
        return true;
    }

    if (try_consume_if(ctx)) {
        return true;
    }

    if (try_consume_while(ctx)) {
        return true;
    }

    if (try_consume_block(ctx)) {
        return true;
    }

    if (try_consume_discard(ctx)) {
        return true;
    }

    return false;
}

static bool try_consume_block(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *start_token;
    if (!try_consume_token(&new_ctx, TOKEN_LBRACE, &start_token)) {
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
        .source_loc = start_token->source_loc,
        .as.block = stmts,
    };
    ctx_update(ctx, &new_ctx, &block_node);

    return true;
}

static bool try_consume_param(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    token_t *dots_token;
    if (try_consume_token(&new_ctx, TOKEN_DOTS, &dots_token)) {
        node_t vararg_node = {
            .type = NODE_VAR_DECL,
            .source_loc = dots_token->source_loc,
            .as.var_decl.is_varargs = true,
        };
        ctx_update(ctx, &new_ctx, &vararg_node);

        return true;
    }

    if (!try_consume_type(&new_ctx)) {
        return false;
    }
    node_ref_t type_ref = ctx_get_result_ref(&new_ctx);

    token_t *identifier_token;
    if (!try_consume_token(&new_ctx, TOKEN_IDENTIFIER, &identifier_token)) {
        return false;
    }

    node_t param_node = {
        .type = NODE_VAR_DECL,
        .source_loc = node_ref_get(type_ref)->source_loc,
        .as.var_decl.type_ref = type_ref,
        .as.var_decl.name = identifier_token,
    };
    ctx_update(ctx, &new_ctx, &param_node);

    trace("try_consume_param succeeded\n");
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

    node_t func_sig_node = {
        .type = NODE_FUNCTION_SIGNATURE,
        .source_loc = node_ref_get(return_type_ref)->source_loc,
        .as.function_signature.return_type_ref = return_type_ref,
        .as.function_signature.name = identifier_token,
    };

    if (!try_consume_token(&new_ctx, TOKEN_LPAREN, NULL)) {
        return false;
    }

    list_t parameters = { .element_size = sizeof(node_ref_t) };

    // f(void) case
    if (
        new_ctx.token_view.length > 0 && lv_at(&new_ctx.token_view, token_t, 0)->type == TOKEN_VOID
        && new_ctx.token_view.length > 1 && lv_at(&new_ctx.token_view, token_t, 1)->type == TOKEN_RPAREN
    ) {
        try_consume_token(&new_ctx, TOKEN_VOID, NULL);
    } else {
        bool trailing_comma = false;
        while (true) {
            if (!try_consume_param(&new_ctx)) {
                break;
            }
            trailing_comma = false;

            node_ref_t param_ref = ctx_get_result_ref(&new_ctx);
            list_push(&parameters, &param_ref);

            if (!try_consume_token(&new_ctx, TOKEN_COMMA, NULL)) {
                break;
            }
            trailing_comma = true;
        }
        if (trailing_comma) {
            return false;
        }
    }

    // Ensure varargs is at end if it exists
    for (size_t i = 0; i < parameters.length; i++) {
        node_ref_t *param_ref = list_at(&parameters, node_ref_t, i);
        node_t *param = node_ref_get(*param_ref);
        assert(param->type == NODE_VAR_DECL);

        if (param->as.var_decl.is_varargs && i != parameters.length - 1) {
            report_error(param->source_loc, "Varargs can only come after the last parameter");
        }
    }

    func_sig_node.as.function_signature.parameters = parameters;

    if (!try_consume_token(&new_ctx, TOKEN_RPAREN, NULL)) {
        list_clear(&parameters);
        return false;
    }

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
        .source_loc = node_ref_get(func_sig_ref)->source_loc,
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

bool try_consume_top_level(parse_ctx_t *ctx) {
    return try_consume_function(ctx);
}

bool try_consume_file(parse_ctx_t *ctx) {
    parse_ctx_t new_ctx = *ctx;

    list_t top_levels = { .element_size = sizeof(node_ref_t) };
    while (true) {
        if (!try_consume_top_level(&new_ctx)) {
            break;
        }
        node_ref_t top_level_ref = ctx_get_result_ref(&new_ctx);
        list_push(&top_levels, &top_level_ref);
    }

    node_t file_node = {
        .type = NODE_FILE,
        .as.file.top_levels = top_levels,
    };
    ctx_update(ctx, &new_ctx, &file_node);

    return true;
}

bool parse(list_t *nodes, list_t *tokens, node_ref_t *root_ref) {
    root_ref->nodes = nodes;
    parse_ctx_t ctx = {
        .nodes = nodes,
        .token_view = lv_from_list(tokens),
        .result_index = &root_ref->index,
    };

    if (!try_consume_file(&ctx)) {
        return false;
    }

    if (ctx.token_view.length != 0) {
        return false;
    }

    return true;
}
