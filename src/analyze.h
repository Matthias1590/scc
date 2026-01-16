#pragma once

#include "scc.h"

typedef struct {
	bool global;
	bool is_forward_decl;
	token_t *name;
	node_ref_t type_ref;
	size_t scope_depth;
} symbol_t;

bool analyze(node_ref_t root_ref, const char *out_path);
