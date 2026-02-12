#pragma once

#include "scc.h"

typedef enum {
    NODE_INTLIT,
    NODE_STRINGLIT,
    NODE_CHARLIT,
    NODE_ADD,
    NODE_SUB,
    NODE_MULT,
    NODE_DIV,
    NODE_VAR_DECL,
    NODE_BLOCK,
    NODE_FLOAT,
    NODE_INT,
    NODE_LONG,
    NODE_CHAR,
    NODE_VOID,
    NODE_PTR_TYPE,
    NODE_ASSIGNMENT,
    NODE_IDENTIFIER,
    NODE_RETURN,
    NODE_FUNCTION,
    NODE_FUNCTION_SIGNATURE,
    NODE_CAST,
    NODE_ADDRESS_OF,
    NODE_DEREF,
    NODE_IF,
    NODE_NEQ,
    NODE_FILE,
    NODE_CALL,
    NODE_EQEQ,
    NODE_GT,
    NODE_LT,
    NODE_LTE,
    NODE_PLUSEQ,
    NODE_DISCARD,
    NODE_WHILE,
    NODE_NEGATE,
    NODE_INDEX,
    NODE_POSTINC,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_EMPTY_STMT,
} node_type_t;

typedef struct node_t node_t;

typedef struct {
    list_t *nodes;
    size_t index;
} node_ref_t;

struct node_t {
    source_loc_t source_loc;
    node_type_t type;
    union {
        token_t intlit;
        token_t stringlit;
        token_t charlit;
        struct {
            node_ref_t left_ref;
            node_ref_t right_ref;
        } binop;
        struct {
            node_ref_t type_ref;
            token_t *name;
            node_ref_t init_expr_ref;
            bool is_array;
            node_ref_t array_size_expr_ref;
            bool is_varargs;
        } var_decl;
        struct {
            node_ref_t signature_ref;
            node_ref_t body_ref;
        } function;
        struct {
            node_ref_t return_type_ref;
            token_t *name;
            list_t parameters;
        } function_signature;
        struct {
            node_ref_t expr_ref;
        } ret;
        struct {
            node_ref_t expr_ref;
            node_ref_t target_type_ref;
        } cast;
        struct {
            node_ref_t base_type_ref;
        } ptr_type;
        struct {
            node_ref_t expr_ref;
        } address_of;
        struct {
            node_ref_t expr_ref;
        } deref;
        struct {
            node_ref_t expr_ref;
            node_ref_t then_ref;
            node_ref_t else_ref;
        } if_;
        struct {
            node_ref_t expr_ref;
            node_ref_t body_ref;
        } while_;
        struct {
            list_t top_levels;
        } file;
        struct {
            node_ref_t function_ref;
            list_t arg_refs;
        } call;
        // TODO: Put these in some unaryop struct
        struct {
            node_ref_t expr_ref;
        } discard;
        struct {
            node_ref_t expr_ref;
        } negate;
        token_t identifier;
        list_t block;
        struct {
            bool is_signed;
        } type;
        struct {
            node_ref_t expr_ref;
            node_ref_t index_ref;
        } index;
        struct {
            node_ref_t expr_ref;
        } postinc;
    } as;
};

void node_print(node_ref_t ref);
bool parse(list_t *nodes, list_t *tokens, node_ref_t *root_ref);
node_t *node_ref_get(node_ref_t ref);
bool node_ref_is_null(node_ref_t ref);
