#pragma once

#include "scc.h"

typedef struct {
    const char *string;
    size_t length;
} sv_t;

sv_t sv_from_cstr(const char *cstr);
sv_t sv_trim_left(sv_t sv);
sv_t sv_take(sv_t sv, size_t n);
sv_t sv_consume(sv_t *sv, size_t n);
char *sv_to_cstr(sv_t sv, char *buffer, size_t buffer_size);
bool sv_eq(sv_t a, sv_t b);
