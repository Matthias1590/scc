int fact(int x) {
    if (x == 0) {
        return 1;
    } else {
        return x * fact(x - 1);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }
    return fact(5);
}
