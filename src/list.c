#include "scc.h"

void *list_at_raw(list_t *list, size_t index) {
    return (void *)((char *)list->element_bytes + (index * list->element_size));
}

void list_push(list_t *list, void *item) {
    if (list->length >= list->capacity) {
        size_t new_capacity = (list->capacity == 0) ? 1 : list->capacity * 2;
        void *new_bytes = realloc(list->element_bytes, new_capacity * list->element_size);
        assert(new_bytes != NULL);
        list->element_bytes = new_bytes;
        list->capacity = new_capacity;
    }
    memcpy(list_at_raw(list, list->length), item, list->element_size);
    list->length++;
}

void list_clear(list_t *list) {
    free(list->element_bytes);
    list->element_bytes = NULL;
    list->length = 0;
    list->capacity = 0;
}

void *lv_at_raw(lv_t *lv, size_t index) {
    return list_at_raw(lv->list, lv->start + index);
}

lv_t lv_from_list(list_t *list) {
    return (lv_t){ .list = list, .start = 0, .length = list->length };
}
