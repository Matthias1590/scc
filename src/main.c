#include "scc.h"

char *preprocess_file(const char *in_path) {
    // Run `cpp -x c {PATH} | grep -v "^#"`
    char command[512];
    snprintf(command, sizeof(command), "cpp -x c \"%s\" | grep -v \"^#\"", in_path);

    FILE *pipe = popen(command, "r");
    if (pipe == NULL) {
        todo("Handle preprocess error");
    }

    char *buf = NULL;
    size_t len = 0;
    FILE *mem = open_memstream(&buf, &len);

    char tmp[4096];
    while (fgets(tmp, sizeof tmp, pipe))
        fputs(tmp, mem);

    pclose(pipe);
    fclose(mem);   // finalizes buffer
    // buf is NUL-terminated, length in len

    return buf;
}

int main(void) {
    char *in_path = "test.c";
    char *out_path = "test.qbe";

    char *code = preprocess_file(in_path);
    // char *code = read_file(in_path);
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
