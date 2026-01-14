int f(int x) {
    return x - 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        return 1;
    }
    return f(5);
}
