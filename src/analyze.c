#include "scc.h"

static symbol_t *find_symbol(list_t symbol_map, sv_t name) {
	for (size_t i = 0; i < symbol_map.length; i++) {
		symbol_t *symbol = list_at(&symbol_map, symbol_t, i);
		if (sv_eq(sv_from_cstr(symbol->name->as.identifier), name)) {
			return symbol;
		}
	}
	return NULL;
}

static symbol_t *find_symbol_recursive(list_t *symbol_maps, sv_t name) {
	for (ssize_t i = symbol_maps->length - 1; i >= 0; i--) {
		list_t *symbol_map = list_at(symbol_maps, list_t, i);
		symbol_t *symbol = find_symbol(*symbol_map, name);
		if (symbol != NULL) {
			return symbol;
		}
	}
	return NULL;
}

static void push_map(list_t *symbol_maps) {
	list_t symbol_map = { .element_size = sizeof(symbol_t) };
	list_push(symbol_maps, &symbol_map);
}

static void pop_map(list_t *symbol_maps) {
	assert(symbol_maps->length > 0);

	list_t *symbol_map = list_at(symbol_maps, list_t, symbol_maps->length - 1);
	list_clear(symbol_map);
	symbol_maps->length--;
}

static void add_symbol(list_t *symbol_maps, symbol_t symbol) {
	assert(symbol_maps->length > 0);
	list_t *current_map = list_at(symbol_maps, list_t, symbol_maps->length - 1);

	if (find_symbol(*current_map, sv_from_cstr(symbol.name->as.identifier)) != NULL) {
		todo("Report redeclaration error");
	}

	list_push(current_map, &symbol);
}

bool analyze_node(list_t *symbol_maps, node_ref_t node_ref) {
	node_t *node = node_ref_get(node_ref);

	switch (node->type) {
		case NODE_BLOCK:
			push_map(symbol_maps);  // TODO: This shouldnt happen on function bodies, you cant shadow parameters directly in the function body
			for (size_t i = 0; i < node->as.block.length; i++) {
				node_ref_t *child_ref = list_at(&node->as.block, node_ref_t, i);
				if (!analyze_node(symbol_maps, *child_ref)) {
					return false;
				}
			}
			pop_map(symbol_maps);
			return true;
		case NODE_VAR_DECL:
			add_symbol(symbol_maps, (symbol_t) {
				.name = node->as.var_decl.name,
				.type_ref = node->as.var_decl.type_ref,
			});
			return true;
		case NODE_ADD:
		case NODE_SUB:
		case NODE_MULT:
		case NODE_DIV:
		case NODE_ASSIGNMENT:
			return analyze_node(symbol_maps, node->as.binop.left_ref)
				&& analyze_node(symbol_maps, node->as.binop.right_ref);
		case NODE_INTLIT:
			return true;
		case NODE_IDENTIFIER:
			if (!find_symbol_recursive(symbol_maps, sv_from_cstr(node->as.identifier.as.identifier))) {
				todo("Report undeclared identifier error");
			}
			return true;
		case NODE_FUNCTION:
			add_symbol(symbol_maps, (symbol_t) {
				.name = node_ref_get(node->as.function.signature_ref)->as.function_signature.name,
				.type_ref = node->as.function.signature_ref,
			});
			// TODO: Define parameters in new scope
			return analyze_node(symbol_maps, node->as.function.body_ref);
		default:
			unreachable();
	}
}

bool analyze(node_ref_t root_ref) {
	list_t symbol_maps = { .element_size = sizeof(list_t) };
	push_map(&symbol_maps);

	return analyze_node(&symbol_maps, root_ref);
}
