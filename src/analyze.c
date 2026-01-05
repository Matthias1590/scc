#include "scc.h"

static node_t int_type = {
	.type = NODE_INT,
};
static node_t void_type = {
	.type = NODE_VOID,
};

typedef enum {
	QBE_VAR_IDENTIFIER,
	QBE_VAR_TEMP,
} qbe_var_type_t;

typedef enum {
	QBE_VALUE_VOID,
	QBE_VALUE_WORD,
	QBE_VALUE_SINGLE,
} qbe_value_type_t;

typedef struct {
	bool global;
	qbe_var_type_t var_type;
	qbe_value_type_t value_type;
	union {	
		char *identifier;
		size_t temp;
	} as;
} qbe_var_t;

typedef struct {
	FILE *out_file;
	qbe_var_t result_var;
	node_t *result_type;
	node_ref_t function_return_type_ref;
	size_t next_temp;
} codegen_ctx_t;

static qbe_var_t ctx_new_temp(codegen_ctx_t *ctx, qbe_value_type_t value_type) {
	qbe_var_t temp_var = {
		.global = false,
		.var_type = QBE_VAR_TEMP,
		.value_type = value_type,
		.as.temp = ctx->next_temp++,
	};
	return temp_var;
}

static void qbe_write_type(codegen_ctx_t *ctx, qbe_value_type_t value_type) {
	switch (value_type) {
		case QBE_VALUE_VOID:
			break;
		case QBE_VALUE_WORD:
			fprintf(ctx->out_file, "w ");
			break;
		case QBE_VALUE_SINGLE:
			fprintf(ctx->out_file, "s ");
			break;
	}
}

static void qbe_write_var(codegen_ctx_t *ctx, qbe_var_t var) {
	if (var.global) {
		fprintf(ctx->out_file, "$");
	} else {
		fprintf(ctx->out_file, "%%");
	}

	switch (var.var_type) {
		case QBE_VAR_IDENTIFIER:
			if (!var.global) {
				fprintf(ctx->out_file, "ident_");
			}
			fprintf(ctx->out_file, "%s", var.as.identifier);
			break;
		case QBE_VAR_TEMP:
			assert(!var.global);
			fprintf(ctx->out_file, "temp_%zu", var.as.temp);
			break;
	}
}

static qbe_value_type_t qbe_type_from_node(node_t *node) {
	switch (node->type) {
	case NODE_VOID:
		return QBE_VALUE_VOID;
	case NODE_INT:
		return QBE_VALUE_WORD;
	case NODE_FLOAT:
		return QBE_VALUE_SINGLE;
	default:
		todo("Unhandled type conversion from node to qbe type");
	}
}

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

// 	node_t *node = node_ref_get(node_ref);

// 	switch (node->type) {
	// 	case NODE_ASSIGNMENT: {
		// 		if (!gen_node(ctx, node->as.binop.right_ref)) {
			// 			return false;
// 		}
// 		qbe_var_t right_var = ctx->result_var;
// 		if (!gen_node(ctx, node->as.binop.left_ref)) {
	// 			return false;
	// 		}
	// 		qbe_write_var(ctx, ctx->result_var);
	// 		fprintf(ctx->out_file, " =");
	// 		qbe_write_type(ctx, qbe_type_from_node(node_get_type(node->as.binop.left_ref)));
	// 	} break;
	// 	default:
	// 		todo("Unhandled node type in codegen");
	// 	}
	
	// 	return true;

bool is_global_map(list_t *symbol_maps) {
	return symbol_maps->length == 1;
}

static bool type_eq(node_t *a, node_t *b) {
	// TODO: This only works for primitives
	return a->type == b->type;
}

bool analyze_node(codegen_ctx_t *ctx, list_t *symbol_maps, node_ref_t node_ref) {
	node_t *node = node_ref_get(node_ref);

	switch (node->type) {
		case NODE_BLOCK:
			push_map(symbol_maps);  // TODO: This shouldnt happen on function bodies, you cant shadow parameters directly in the function body
			for (size_t i = 0; i < node->as.block.length; i++) {
				node_ref_t *child_ref = list_at(&node->as.block, node_ref_t, i);
				if (!analyze_node(ctx, symbol_maps, *child_ref)) {
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
			if (!node_ref_is_null(node->as.var_decl.init_expr_ref)) {
				// TODO: Analyze init expression type compatibility

				if (!analyze_node(ctx, symbol_maps, node->as.var_decl.init_expr_ref)) {
					return false;
				}
				fprintf(ctx->out_file, "    ");
				qbe_write_var(ctx, (qbe_var_t) {
					.global = false,
					.var_type = QBE_VAR_IDENTIFIER,
					.value_type = qbe_type_from_node(node_ref_get(node->as.var_decl.type_ref)),
					.as.identifier = node->as.var_decl.name->as.identifier,
				});
				fprintf(ctx->out_file, " =");
				qbe_write_type(ctx, qbe_type_from_node(node_ref_get(node->as.var_decl.type_ref)));
				fprintf(ctx->out_file, "copy ");
				qbe_write_var(ctx, ctx->result_var);
				fprintf(ctx->out_file, "\n");
			}
			return true;
		case NODE_ASSIGNMENT: {
			// TODO: Assigning returns the new value and can be used as an expression

			if (!analyze_node(ctx, symbol_maps, node->as.binop.left_ref)) {
				return false;
			}
			qbe_var_t left_var = ctx->result_var;
			node_t *left_type = ctx->result_type;
			if (!analyze_node(ctx, symbol_maps, node->as.binop.right_ref)) {
				return false;
			}
			qbe_var_t right_var = ctx->result_var;
			// node_t *right_type = ctx->result_type;

			// TODO: Ensure left_type and right_type are compatible

			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, left_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, qbe_type_from_node(left_type));
			if (qbe_type_from_node(left_type) == qbe_type_from_node(ctx->result_type)) {
				fprintf(ctx->out_file, "copy ");
			} else {
				fprintf(ctx->out_file, "cast ");
			}
			qbe_write_var(ctx, right_var);
			fprintf(ctx->out_file, "\n");
		} break;
		case NODE_ADD:
		case NODE_SUB:
		case NODE_MULT:
		case NODE_DIV: {
			if (!analyze_node(ctx, symbol_maps, node->as.binop.left_ref)) {
				return false;
			}
			qbe_var_t left_var = ctx->result_var;
			node_t *left_type = ctx->result_type;
			if (!analyze_node(ctx, symbol_maps, node->as.binop.right_ref)) {
				return false;
			}
			qbe_var_t right_var = ctx->result_var;
			// node_t *right_type = ctx->result_type;

			// TODO: Ensure left_type and right_type are compatible

			qbe_var_t result_var = ctx_new_temp(ctx, qbe_type_from_node(left_type));
			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, result_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, qbe_type_from_node(left_type));
			switch (node->type) {
				case NODE_ADD:
					fprintf(ctx->out_file, "add ");
					break;
				case NODE_SUB:
					fprintf(ctx->out_file, "sub ");
					break;
				case NODE_MULT:
					fprintf(ctx->out_file, "mul ");
					break;
				case NODE_DIV:
					fprintf(ctx->out_file, "div ");
					break;
				default:
					unreachable();
			}
			qbe_write_var(ctx, left_var);
			fprintf(ctx->out_file, ", ");
			qbe_write_var(ctx, right_var);
			fprintf(ctx->out_file, "\n");

			ctx->result_var = result_var;
			ctx->result_type = left_type;  // TODO: This assumes both sides have the same type, no promotion here, fix that
		} break;
		case NODE_INTLIT:
			ctx->result_var = ctx_new_temp(ctx, QBE_VALUE_WORD);
			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, ctx->result_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, QBE_VALUE_WORD);
			fprintf(ctx->out_file, "copy %d\n", node->as.intlit.as.intlit);
			ctx->result_type = &int_type;
			return true;
		case NODE_IDENTIFIER: {
			symbol_t *symbol = find_symbol_recursive(symbol_maps, sv_from_cstr(node->as.identifier.as.identifier));
			if (!symbol) {
				todo("Report undeclared identifier error");
			}

			ctx->result_var = (qbe_var_t) {
				.global = is_global_map(symbol_maps),
				.var_type = QBE_VAR_IDENTIFIER,
				.value_type = qbe_type_from_node(node_ref_get(symbol->type_ref)),
				.as.identifier = symbol->name->as.identifier,
			};
			ctx->result_type = node_ref_get(symbol->type_ref);
		} break;
		case NODE_FUNCTION: {
			node_t *signature_node = node_ref_get(node->as.function.signature_ref);
			node_t *return_type_node = node_ref_get(signature_node->as.function_signature.return_type_ref);
			ctx->function_return_type_ref = signature_node->as.function_signature.return_type_ref;

			add_symbol(symbol_maps, (symbol_t) {
				.name = signature_node->as.function_signature.name,
				.type_ref = node->as.function.signature_ref,
			});

			fprintf(ctx->out_file, "export function ");
			qbe_write_type(ctx, qbe_type_from_node(return_type_node));
			// TODO: Handle parameters
			qbe_write_var(ctx, (qbe_var_t) {
				.global = true,
				.var_type = QBE_VAR_IDENTIFIER,
				.as.identifier = signature_node->as.function_signature.name->as.identifier,
			});
			fprintf(ctx->out_file, "() {\n");
			fprintf(ctx->out_file, "@start\n");
			if (!analyze_node(ctx, symbol_maps, node->as.function.body_ref)) {
				return false;
			}
			if (return_type_node->type == NODE_VOID) {
				fprintf(ctx->out_file, "    ret\n");
			}
			fprintf(ctx->out_file, "}\n");
		} break;
		case NODE_RETURN: {
			node_t *function_return_type = node_ref_get(ctx->function_return_type_ref);

			node_t *expr_type = &void_type;
			if (!node_ref_is_null(node->as.ret.expr_ref)) {
				if (!analyze_node(ctx, symbol_maps, node->as.ret.expr_ref)) {
					return false;
				}
				expr_type = ctx->result_type;
			}

			// TODO: Implicit casting should be handled somewhere, probably not here though
			if (!type_eq(function_return_type, expr_type)) {
				todo("Report return type mismatch error");
			}

			// Early return for void functions
			if (expr_type->type == NODE_VOID) {
				fprintf(ctx->out_file, "    ret\n");
				return true;
			}

			fprintf(ctx->out_file, "    ret ");
			qbe_write_var(ctx, ctx->result_var);
			fprintf(ctx->out_file, "\n");
		} break;
		default:
			unreachable();
	}

	return true;
}

bool analyze(node_ref_t root_ref, const char *out_path) {
	FILE *out_file = fopen(out_path, "w");
	if (out_file == NULL) {
		todo("Handle file open error");
	}

	codegen_ctx_t ctx = {
		.out_file = out_file,
	};

	list_t symbol_maps = { .element_size = sizeof(list_t) };
	push_map(&symbol_maps);

	bool success = analyze_node(&ctx, &symbol_maps, root_ref);

	fclose(out_file);
	return success;
}
