void inc(int *p) {
    *p += 1;
}

int main() {
    int x = 41;
    inc(&x);
    if (x == 42) {
        return 0;
    }
    return 1;
}
