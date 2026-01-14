#include "scc.h"

typedef enum {
	TYPE_INT,
	TYPE_LONG,
	TYPE_VOID,
	TYPE_CHAR,
	TYPE_FUNC,
} type_kind_t;

typedef struct {
	type_kind_t kind;
	size_t pointer_depth;
} type_t;

static type_t int_type = {
	.kind = TYPE_INT,
	.pointer_depth = 0,
};
static type_t long_type = {
	.kind = TYPE_LONG,
	.pointer_depth = 0,
};
static type_t void_type = {
	.kind = TYPE_VOID,
	.pointer_depth = 0,
};
static type_t char_type = {
	.kind = TYPE_CHAR,
	.pointer_depth = 0,
};
static type_t func_type = {
	.kind = TYPE_FUNC,
	.pointer_depth = 0,
};

static type_t type_from_node(node_t *node) {
	switch (node->type) {
	case NODE_INT:
		return int_type;
	case NODE_LONG:
		return long_type;
	case NODE_VOID:
		return void_type;
	case NODE_PTR_TYPE: {
		type_t base_type = type_from_node(node_ref_get(node->as.ptr_type.base_type_ref));
		base_type.pointer_depth++;
		return base_type;
	}
	case NODE_CHAR: {
		return char_type;
	}
	case NODE_FUNCTION_SIGNATURE: {
		return func_type;
	}
	default:
		todo("Unhandled type conversion from node to type");
	}
}

typedef enum {
	QBE_VAR_IDENTIFIER,
	QBE_VAR_TEMP,
	QBE_VAR_PARAM,
} qbe_var_type_t;

typedef enum {
	QBE_VALUE_VOID,
	QBE_VALUE_WORD,
	QBE_VALUE_LONG,
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
	size_t label_num;
} qbe_label_t;

typedef struct {
	FILE *out_file;
	qbe_var_t result_var;
	type_t result_type;
	type_t function_return_type;
	size_t next_label;
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

static qbe_label_t ctx_new_label(codegen_ctx_t *ctx) {
	qbe_label_t label = {
		.label_num = ctx->next_label++,
	};
	return label;
}

static void qbe_write_type(codegen_ctx_t *ctx, qbe_value_type_t value_type) {
	switch (value_type) {
		case QBE_VALUE_VOID:
			break;
		case QBE_VALUE_WORD:
			fprintf(ctx->out_file, "w ");
			break;
		case QBE_VALUE_LONG:
			fprintf(ctx->out_file, "l ");
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
		case QBE_VAR_PARAM:
			assert(!var.global);
			fprintf(ctx->out_file, "param_%s", var.as.identifier);
			break;
	}
}

static qbe_value_type_t qbe_type_from_type(type_t type) {
	if (type.pointer_depth > 0) {
		return QBE_VALUE_LONG;
	}
	switch (type.kind) {
	case TYPE_INT:
		return QBE_VALUE_WORD;
	case TYPE_LONG:
		return QBE_VALUE_LONG;
	case TYPE_VOID:
		return QBE_VALUE_VOID;
	case TYPE_FUNC:
		return QBE_VALUE_LONG;
	default:
		todo("Unhandled type conversion from type to qbe type");
	}
}

static size_t qbe_type_size(qbe_value_type_t value_type) {
	switch (value_type) {
		case QBE_VALUE_VOID:
			return 0;
		case QBE_VALUE_WORD:
			return 4;
		case QBE_VALUE_SINGLE:
			return 4;
		case QBE_VALUE_LONG:
			return 8;
		default:
			unreachable();
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

bool is_global_map(list_t *symbol_maps) {
	return symbol_maps->length == 1;
}

static bool type_eq(type_t a, type_t b) {
	return a.kind == b.kind && a.pointer_depth == b.pointer_depth;
}

bool analyze_node(codegen_ctx_t *ctx, list_t *symbol_maps, node_ref_t node_ref, bool emit_lvalue) {
	node_t *node = node_ref_get(node_ref);

	switch (node->type) {
		case NODE_BLOCK:
			push_map(symbol_maps);  // TODO: This shouldnt happen on function bodies, you cant shadow parameters directly in the function body
			for (size_t i = 0; i < node->as.block.length; i++) {
				node_ref_t *child_ref = list_at(&node->as.block, node_ref_t, i);
				if (!analyze_node(ctx, symbol_maps, *child_ref, false)) {
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
			type_t type = type_from_node(node_ref_get(node->as.var_decl.type_ref));
			qbe_var_t var = (qbe_var_t) {
				.global = is_global_map(symbol_maps),
				.var_type = QBE_VAR_IDENTIFIER,
				.value_type = qbe_type_from_type(type),
				.as.identifier = node->as.var_decl.name->as.identifier,
			};

			// Allocate stack space
			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, var);
			fprintf(ctx->out_file, " =l alloc4 %zu\n", qbe_type_size(var.value_type));

			if (!node_ref_is_null(node->as.var_decl.init_expr_ref)) {
				// TODO: Analyze init expression type compatibility

				if (!analyze_node(ctx, symbol_maps, node->as.var_decl.init_expr_ref, false)) {
					return false;
				}
				fprintf(ctx->out_file, "    ");
				fprintf(ctx->out_file, "store");
				qbe_write_type(ctx, qbe_type_from_type(type));
				qbe_write_var(ctx, ctx->result_var);
				fprintf(ctx->out_file, ", ");
				qbe_write_var(ctx, var);
				fprintf(ctx->out_file, "\n");
			}
			return true;
		case NODE_ASSIGNMENT: {
			if (!analyze_node(ctx, symbol_maps, node->as.binop.left_ref, true)) {
				return false;
			}
			qbe_var_t left_var = ctx->result_var;
			type_t left_type = ctx->result_type;
			if (!analyze_node(ctx, symbol_maps, node->as.binop.right_ref, false)) {
				return false;
			}
			qbe_var_t right_var = ctx->result_var;

			// TODO: Ensure left_type and right_type are compatible

			fprintf(ctx->out_file, "    ");
			fprintf(ctx->out_file, "store");;
			qbe_write_type(ctx, qbe_type_from_type(left_type));
			qbe_write_var(ctx, right_var);
			fprintf(ctx->out_file, ", ");
			qbe_write_var(ctx, left_var);
			fprintf(ctx->out_file, "\n");
		} break;
		case NODE_ADD:
		case NODE_SUB:
		case NODE_MULT:
		case NODE_DIV: {
			if (!analyze_node(ctx, symbol_maps, node->as.binop.left_ref, false)) {
				return false;
			}
			qbe_var_t left_var = ctx->result_var;
			type_t left_type = ctx->result_type;
			if (!analyze_node(ctx, symbol_maps, node->as.binop.right_ref, false)) {
				return false;
			}
			qbe_var_t right_var = ctx->result_var;
			// node_t *right_type = ctx->result_type;

			// TODO: Ensure left_type and right_type are compatible

			qbe_var_t result_var = ctx_new_temp(ctx, qbe_type_from_type(left_type));
			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, result_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, qbe_type_from_type(left_type));
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
			ctx->result_type = int_type;
			return true;
		case NODE_IDENTIFIER: {
			symbol_t *symbol = find_symbol_recursive(symbol_maps, sv_from_cstr(node->as.identifier.as.identifier));
			if (!symbol) {
				fprintf(stderr, "Undeclared identifier: '%s'\n", node->as.identifier.as.identifier);
				printf("Available symbols:\n");
				for (ssize_t i = symbol_maps->length - 1; i >= 0; i--) {
					list_t *symbol_map = list_at(symbol_maps, list_t, i);
					for (size_t j = 0; j < symbol_map->length; j++) {
						symbol_t *sym = list_at(symbol_map, symbol_t, j);
						printf("- %s\n", sym->name->as.identifier);
					}
				}
				exit(1);
			}

			type_t type = type_from_node(node_ref_get(symbol->type_ref));
			qbe_var_t var = (qbe_var_t) {
				.global = is_global_map(symbol_maps),
				.var_type = QBE_VAR_IDENTIFIER,
				.value_type = qbe_type_from_type(type),
				.as.identifier = symbol->name->as.identifier,
			};

			if (emit_lvalue) {
				// Just return the address
				ctx->result_var = var;
				ctx->result_type = type;
			} else {
				// Deref
				qbe_var_t temp = ctx_new_temp(ctx, qbe_type_from_type(type));
				fprintf(ctx->out_file, "    ");
				qbe_write_var(ctx, temp);
				fprintf(ctx->out_file, " =");
				qbe_write_type(ctx, qbe_type_from_type(type));
				fprintf(ctx->out_file, "load");
				qbe_write_type(ctx, qbe_type_from_type(type));
				qbe_write_var(ctx, var);
				fprintf(ctx->out_file, "\n");

				ctx->result_var = temp;
				ctx->result_type = type_from_node(node_ref_get(symbol->type_ref));
			}
		} break;
		case NODE_FUNCTION: {
			node_t *signature_node = node_ref_get(node->as.function.signature_ref);
			node_t *return_type_node = node_ref_get(signature_node->as.function_signature.return_type_ref);
			ctx->function_return_type = type_from_node(return_type_node);

			add_symbol(symbol_maps, (symbol_t) {
				.name = signature_node->as.function_signature.name,
				.type_ref = node->as.function.signature_ref,
			});

			fprintf(ctx->out_file, "export function ");
			qbe_write_type(ctx, qbe_type_from_type(ctx->function_return_type));
			qbe_write_var(ctx, (qbe_var_t) {
				.global = true,
				.var_type = QBE_VAR_IDENTIFIER,
				.as.identifier = signature_node->as.function_signature.name->as.identifier,
			});

			push_map(symbol_maps);

			fprintf(ctx->out_file, "(");
			for (size_t i = 0; i < signature_node->as.function_signature.parameters.length; i++) {
				node_ref_t *param_ref = list_at(&signature_node->as.function_signature.parameters, node_ref_t, i);
				node_t *param_node = node_ref_get(*param_ref);
				type_t param_type = type_from_node(node_ref_get(param_node->as.var_decl.type_ref));

				if (i > 0) {
					fprintf(ctx->out_file, ", ");
				}
				qbe_write_type(ctx, qbe_type_from_type(param_type));
				qbe_write_var(ctx, (qbe_var_t) {
					.global = false,
					.var_type = QBE_VAR_PARAM,
					.as.identifier = param_node->as.var_decl.name->as.identifier,
				});

				add_symbol(symbol_maps, (symbol_t) {
					.name = param_node->as.var_decl.name,
					.type_ref = param_node->as.var_decl.type_ref,
				});
			}
			fprintf(ctx->out_file, ")\n{\n");
			fprintf(ctx->out_file, "@start\n");

			// Copy parameters into local stack space
			for (size_t i = 0; i < signature_node->as.function_signature.parameters.length; i++) {
				node_ref_t *param_ref = list_at(&signature_node->as.function_signature.parameters, node_ref_t, i);
				node_t *param_node = node_ref_get(*param_ref);
				type_t param_type = type_from_node(node_ref_get(param_node->as.var_decl.type_ref));

				qbe_var_t param_var = (qbe_var_t) {
					.global = false,
					.var_type = QBE_VAR_IDENTIFIER,
					.value_type = qbe_type_from_type(param_type),
					.as.identifier = param_node->as.var_decl.name->as.identifier,
				};
				qbe_var_t param_input_var = (qbe_var_t) {
					.global = false,
					.var_type = QBE_VAR_PARAM,
					.value_type = qbe_type_from_type(param_type),
					.as.identifier = param_node->as.var_decl.name->as.identifier,
				};

				fprintf(ctx->out_file, "    ");
				qbe_write_var(ctx, param_var);
				fprintf(ctx->out_file, " =l alloc4 %zu\n", qbe_type_size(param_var.value_type));
				fprintf(ctx->out_file, "    store");;
				qbe_write_type(ctx, qbe_type_from_type(param_type));
				qbe_write_var(ctx, param_input_var);
				fprintf(ctx->out_file, ", ");
				qbe_write_var(ctx, param_var);
				fprintf(ctx->out_file, "\n");
			}

			if (!analyze_node(ctx, symbol_maps, node->as.function.body_ref, false)) {
				return false;
			}
			fprintf(ctx->out_file, "@end\n");
			if (type_eq(ctx->function_return_type, void_type)) {
				fprintf(ctx->out_file, "    ret\n");
			} else {
				fprintf(ctx->out_file, "    ret %%result\n");
			}
			fprintf(ctx->out_file, "}\n");

			pop_map(symbol_maps);
		} break;
		case NODE_RETURN: {
			type_t expr_type = void_type;
			if (!node_ref_is_null(node->as.ret.expr_ref)) {
				if (!analyze_node(ctx, symbol_maps, node->as.ret.expr_ref, false)) {
					return false;
				}
				expr_type = ctx->result_type;
			}

			// TODO: Implicit casting should be handled somewhere, probably not here though
			if (!type_eq(ctx->function_return_type, expr_type)) {
				todo("Report return type mismatch error");
			}

			// Write return value to %result
			if (!type_eq(expr_type, void_type)) {
				fprintf(ctx->out_file, "    %%result =");
				qbe_write_type(ctx, qbe_type_from_type(expr_type));
				fprintf(ctx->out_file, "copy ");
				qbe_write_var(ctx, ctx->result_var);
				fprintf(ctx->out_file, "\n");
			}
			fprintf(ctx->out_file, "@unused_%zu\n", ctx->next_label++);  // TODO: This is a hack because every block can only end with 1 jump, we add a label to jumps to ensure there's only 1 jump per block.
			fprintf(ctx->out_file, "    jmp @end\n");
		} break;
		case NODE_CAST: {
			if (!analyze_node(ctx, symbol_maps, node->as.cast.expr_ref, false)) {
				return false;
			}
			qbe_var_t expr_var = ctx->result_var;
			type_t expr_type = ctx->result_type;

			type_t target_type = type_from_node(node_ref_get(node->as.cast.target_type_ref));
			qbe_value_type_t target_qbe_type = qbe_type_from_type(target_type);

			// TODO: Ensure expr_type can be cast to target_type

			qbe_var_t result_var = ctx_new_temp(ctx, target_qbe_type);
			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, result_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, target_qbe_type);
			// If the cast is unnecessary, just copy instead of casting
			if (qbe_type_from_type(target_type) == qbe_type_from_type(expr_type)) {
				fprintf(ctx->out_file, "copy ");
			} else {
				fprintf(ctx->out_file, "cast ");
			}
			qbe_write_var(ctx, expr_var);
			fprintf(ctx->out_file, "\n");

			ctx->result_var = result_var;
			ctx->result_type = target_type;
		} break;
		case NODE_ADDRESS_OF: {
			if (!analyze_node(ctx, symbol_maps, node->as.address_of.expr_ref, true)) {
				return false;
			}

			ctx->result_type.pointer_depth++;
		} break;
		case NODE_DEREF: {
			if (!analyze_node(ctx, symbol_maps, node->as.deref.expr_ref, false)) {
				return false;
			}
			qbe_var_t ptr_var = ctx->result_var;
			qbe_value_type_t result_type = qbe_type_from_type(ctx->result_type);

			qbe_var_t result_var = ctx_new_temp(ctx, result_type);
			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, result_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, result_type);
			fprintf(ctx->out_file, "load");
			qbe_write_type(ctx, result_type);
			qbe_write_var(ctx, ptr_var);
			fprintf(ctx->out_file, "\n");

			ctx->result_var = result_var;
			if (ctx->result_type.pointer_depth == 0) {
				report_error(node->source_loc, "Cannot dereference non-pointer type");
			}
			ctx->result_type.pointer_depth--;
		} break;
		case NODE_NEQ: {
			if (!analyze_node(ctx, symbol_maps, node->as.binop.left_ref, false)) {
				return false;
			}
			qbe_var_t left_var = ctx->result_var;
			type_t left_type = ctx->result_type;
			if (!analyze_node(ctx, symbol_maps, node->as.binop.right_ref, false)) {
				return false;
			}
			qbe_var_t right_var = ctx->result_var;
			type_t right_type = ctx->result_type;

			// TODO: Both should be primitives, otherwise you should still get an error (can't compare structs)
			if (!type_eq(left_type, right_type)) {
				todo("Type mismatch in NEQ operation");
			}

			qbe_var_t result_var = ctx_new_temp(ctx, QBE_VALUE_WORD);
			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, result_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, QBE_VALUE_WORD);
			fprintf(ctx->out_file, "cne");
			qbe_write_type(ctx, qbe_type_from_type(left_type));
			qbe_write_var(ctx, left_var);
			fprintf(ctx->out_file, ", ");
			qbe_write_var(ctx, right_var);
			fprintf(ctx->out_file, "\n");

			ctx->result_var = result_var;
			ctx->result_type = int_type;
		} break;
		case NODE_IF: {
			if (!analyze_node(ctx, symbol_maps, node->as.if_.expr_ref, false)) {
				return false;
			}
			qbe_var_t cond_var = ctx->result_var;

			qbe_label_t then_label = ctx_new_label(ctx);
			qbe_label_t end_label = ctx_new_label(ctx);

			fprintf(ctx->out_file, "@unused_%zu\n", ctx->next_label++);  // TODO: This is a hack because every block can only end with 1 jump, we add a label to jumps to ensure there's only 1 jump per block.
			fprintf(ctx->out_file, "    jnz ");
			qbe_write_var(ctx, cond_var);
			fprintf(ctx->out_file, ", @label_%zu, @label_%zu\n", then_label.label_num, end_label.label_num);  // TODO: Create some qbe_write_label function
			fprintf(ctx->out_file, "@label_%zu\n", then_label.label_num);
			if (!analyze_node(ctx, symbol_maps, node->as.if_.then_ref, false)) {
				return false;
			}
			fprintf(ctx->out_file, "@unused_%zu\n", ctx->next_label++);  // TODO: This is a hack because every block can only end with 1 jump, we add a label to jumps to ensure there's only 1 jump per block.
			fprintf(ctx->out_file, "    jmp @label_%zu\n", end_label.label_num);
			fprintf(ctx->out_file, "@label_%zu\n", end_label.label_num);
		} break;
		case NODE_FILE: {
			for (size_t i = 0; i < node->as.file.top_levels.length; i++) {
				node_ref_t *child_ref = list_at(&node->as.file.top_levels, node_ref_t, i);
				if (!analyze_node(ctx, symbol_maps, *child_ref, false)) {
					return false;
				}
			}
		} break;
		case NODE_CALL: {
			// Analyze arguments
			list_t arg_vars = { .element_size = sizeof(qbe_var_t) };
			list_t arg_types = { .element_size = sizeof(type_t) };
			for (size_t i = 0; i < node->as.call.arg_refs.length; i++) {
				node_ref_t *arg_ref = list_at(&node->as.call.arg_refs, node_ref_t, i);
				if (!analyze_node(ctx, symbol_maps, *arg_ref, false)) {
					return false;
				}
				list_push(&arg_vars, &ctx->result_var);
				list_push(&arg_types, &ctx->result_type);
			}

			if (!analyze_node(ctx, symbol_maps, node->as.call.function_ref, false)) {
				return false;
			}
			qbe_var_t function_var = ctx->result_var;
			// type_t function_type = ctx->result_type;

			// TODO: Ensure function_var is actually a function, get its signature, ensure argument types match
			type_t return_type = int_type;  // TODO: Get from actual signature
			qbe_value_type_t return_qbe_type = qbe_type_from_type(return_type);
			qbe_var_t result_var = ctx_new_temp(ctx, return_qbe_type);

			fprintf(ctx->out_file, "    ");
			qbe_write_var(ctx, result_var);
			fprintf(ctx->out_file, " =");
			qbe_write_type(ctx, return_qbe_type);
			fprintf(ctx->out_file, "call ");;
			qbe_write_var(ctx, function_var);
			fprintf(ctx->out_file, "(");
			for (size_t i = 0; i < arg_vars.length; i++) {
				if (i > 0) {
					fprintf(ctx->out_file, ", ");
				}
				qbe_var_t *arg_var = list_at(&arg_vars, qbe_var_t, i);
				qbe_write_type(ctx, arg_var->value_type);
				qbe_write_var(ctx, *arg_var);
			}
			fprintf(ctx->out_file, ")\n");
		} break;
		default:
			todo("Unhandled node type in analyze_node");
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

	bool success = analyze_node(&ctx, &symbol_maps, root_ref, false);

	fclose(out_file);
	return success;
}
