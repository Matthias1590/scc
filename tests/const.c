int test(const int *x) {
    return *x;
}

int main(void) {
    int zero = 0;
    return test(&zero);
}
