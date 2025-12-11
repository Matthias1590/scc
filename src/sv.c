#include "scc.h"

sv_t sv_from_cstr(const char *cstr) {
    return (sv_t){ .string = cstr, .length = strlen(cstr) };
}

sv_t sv_trim_left(sv_t sv) {
    while (sv.length > 0 && isspace(sv.string[0])) {
        sv.string++;
        sv.length--;
    }
    return sv;
}

sv_t sv_take(sv_t sv, size_t n) {
    if (n > sv.length) {
        n = sv.length;
    }
    sv.length = n;
    return sv;
}

sv_t sv_consume(sv_t *sv, size_t n) {
    if (n > sv->length) {
        n = sv->length;
    }
    sv_t result = { .string = sv->string, .length = n };
    sv->string += n;
    sv->length -= n;
    return result;
}

char *sv_to_cstr(sv_t sv, char *buffer, size_t buffer_size) {
    size_t copy_length = (sv.length < buffer_size - 1) ? sv.length : buffer_size - 1;
    memcpy(buffer, sv.string, copy_length);
    buffer[copy_length] = '\0';
    return buffer;
}

bool sv_eq(sv_t a, sv_t b) {
    if (a.length != b.length) {
        return false;
    }
    return memcmp(a.string, b.string, a.length) == 0;
}
