#pragma once

#include "scc.h"

typedef struct {
    void *element_bytes;
    size_t element_size;
    size_t length;
    size_t capacity;
} list_t;

#define list_at(list, type, index) \
    ((type *)list_at_raw(list, index))

typedef struct {
    list_t *list;
    size_t start;
    size_t length;
} lv_t;

#define lv_at(lv, type, index) \
    ((type *)lv_at_raw(lv, index))

void *list_at_raw(list_t *list, size_t index);
void list_push(list_t *list, void *item);
void list_pop(list_t *list);
void list_clear(list_t *list);
void *lv_at_raw(lv_t *lv, size_t index);
lv_t lv_from_list(list_t *list);
