int main() {
    int a[2];
    // int *a2 = &a[1];
    a[0] = 1;
    a[1] = 2;
    if ((a[0] + a[1]) != 3) {
        return 1;
    }
    return 0;
}
