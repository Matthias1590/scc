#pragma once

#include "scc.h"

typedef enum {
	TYPE_VARARGS,
	TYPE_INT,
	TYPE_UNSIGNED_INT,
	TYPE_LONG,
	TYPE_UNSIGNED_LONG,
	TYPE_VOID,
	TYPE_CHAR,
	TYPE_UNSIGNED_CHAR,
	TYPE_FUNC,
	TYPE_PTR,
	TYPE_ARRAY,
} type_kind_t;

typedef struct type_t type_t;
struct type_t {
	type_kind_t kind;
	union {
		struct {
			type_t *return_type;
			list_t parameter_types;
		} func;
		struct {
			type_t *inner;
		} pointer;
		struct {
			type_t *inner;
			node_ref_t size_expr_ref;
		} array;
	} as;
};

typedef struct {
	bool global;
	bool is_forward_decl;
	token_t *name;
	type_t type;
	size_t scope_depth;
} symbol_t;

bool analyze(node_ref_t root_ref, const char *out_path);
