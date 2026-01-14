#include "scc.h"

int main(void) {
    char *in_path = "test.c";
    char *out_path = "test.qbe";

    char *code = read_file(in_path);
    sv_t code_view = sv_from_cstr(code);

    list_t tokens = { .element_size = sizeof(token_t) };
    if (!tokenize(&tokens, code_view, in_path)) {
        todo("Handle tokenization error");
    }

    for (size_t i = 0; i < tokens.length; i++) {
        token_print(list_at(&tokens, token_t, i));
        printf("\n");
    }

    list_t nodes = { .element_size = sizeof(node_t) };
    node_ref_t root_ref;
    if (!parse(&nodes, &tokens, &root_ref)) {
        todo("Handle parse error");
    }

    // assert(nodes.length > 0);
    // node_print(root_ref);
    // printf("\n");

    if (!analyze(root_ref, out_path)) {
        todo("Handle analysis error");
    }

    list_clear(&nodes);
    list_clear(&tokens);
    free(code);
    return 0;
}
