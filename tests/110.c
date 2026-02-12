#define WIDTH 80
#define STEPS 100

int rule110(int l, int c, int r) {
    if (l == 1 && c == 1 && r == 1) return 0;
    if (l == 1 && c == 1 && r == 0) return 1;
    if (l == 1 && c == 0 && r == 1) return 1;
    if (l == 1 && c == 0 && r == 0) return 0;
    if (l == 0 && c == 1 && r == 1) return 1;
    if (l == 0 && c == 1 && r == 0) return 1;
    if (l == 0 && c == 0 && r == 1) return 1;
    return 0; // 000
}

int main() {
    int *current = calloc(WIDTH, 4);
    int *next = calloc(WIDTH, 4);

    current[WIDTH / 2] = 1;

    for (int step = 0; step < STEPS; step++) {

        for (int i = 0; i < WIDTH; i++) {
            if (current[i] == 1)
                putchar('#');
            else
                putchar(' ');
        }
        putchar('\n');

        for (int i = 0; i < WIDTH; i++) {
            int left;
            int right;

            if (i == 0)
                left = current[WIDTH - 1];
            else
                left = current[i - 1];

            if (i == WIDTH - 1)
                right = current[0];
            else
                right = current[i + 1];

            next[i] = rule110(left, current[i], right);
        }

        for (int i = 0; i < WIDTH; i++)
            current[i] = next[i];
    }

    free(current);
    free(next);
    return 0;
}
