#pragma once

#include "scc.h"

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
    NODE_VOID,
    NODE_ASSIGNMENT,
    NODE_IDENTIFIER,
    NODE_FUNCTION,
    NODE_FUNCTION_SIGNATURE,
} node_type_t;

typedef struct node_t node_t;

typedef struct {
    list_t *nodes;
    size_t index;
} node_ref_t;

struct node_t {
    node_type_t type;
    union {
        token_t intlit;
        struct {
            node_ref_t left_ref;
            node_ref_t right_ref;
        } binop;
        struct {
            node_ref_t type_ref;
            token_t *name;
            node_ref_t init_expr_ref;
        } var_decl;
        struct {
            node_ref_t signature_ref;
            node_ref_t body_ref;
        } function;
        struct {
            node_ref_t return_type_ref;
            token_t *name;
        } function_signature;
        token_t identifier;
        list_t block;
    } as;
};

void node_print(node_ref_t ref);
bool parse(list_t *nodes, list_t *tokens, node_ref_t *root_ref);
node_t *node_ref_get(node_ref_t ref);
bool node_ref_is_null(node_ref_t ref);
