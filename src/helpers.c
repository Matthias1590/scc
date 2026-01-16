#include "scc.h"

char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    assert(file != NULL);

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    assert(buffer != NULL);

    size_t read_size = fread(buffer, 1, file_size, file);
    assert(read_size == (size_t)file_size);
    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

void *_heapify(size_t size, const void *value) {
    void *ptr = malloc(size);
    assert(ptr != NULL);
    memcpy(ptr, value, size);
    return ptr;
}
