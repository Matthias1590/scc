#include <stdlib.h>

int sum(char *xs, size_t n) {
    int total = 0;
    for (size_t i = 0; i < n; i++) {
        total += xs[i];
    }
    return total;
}

int main(void) {
    char *xs = malloc(10);
    for (int i = 0; i < 10; i++) {
        xs[i] = i + 1;
    }
    if (sum(xs, 10) != 55) {
        return 1;
    }
    free(xs);
    return 0;
}
