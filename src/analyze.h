#pragma once

#include "scc.h"

typedef struct {
	bool global;
	token_t *name;
	node_ref_t type_ref;
} symbol_t;

bool analyze(node_ref_t root_ref, const char *out_path);
